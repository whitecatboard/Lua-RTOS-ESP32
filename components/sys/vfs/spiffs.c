/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS spiffs vfs
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_SPIFFS

#include "esp_partition.h"

#include <freertos/FreeRTOS.h>

#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <sys/stat.h>

#include "esp_vfs.h"
#include "esp_attr.h"
#include <errno.h>

#include <spiffs.h>
#include <esp_spiffs.h>
#include <spiffs_nucleus.h>
#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/mutex.h>
#include <sys/list.h>
#include <sys/fcntl.h>
#include <sys/vfs/vfs.h>
#include <dirent.h>


static int vfs_spiffs_open(const char *path, int flags, int mode);
static ssize_t vfs_spiffs_write(int fd, const void *data, size_t size);
static ssize_t vfs_spiffs_read(int fd, void * dst, size_t size);
static int vfs_spiffs_fstat(int fd, struct stat * st);
static int vfs_spiffs_close(int fd);
static off_t vfs_spiffs_lseek(int fd, off_t size, int mode);
static int vfs_spiffs_access(const char *path, int amode);

#if !defined(max)
#define max(A,B) ( (A) > (B) ? (A):(B))
#endif

#if !defined(min)
#define min(A,B) ( (A) < (B) ? (A):(B))
#endif

#define VFS_SPIFFS_FLAGS_READONLY  (1 << 0)

static spiffs fs;
static struct list files;

static u8_t *my_spiffs_work_buf = NULL;
static u8_t *my_spiffs_fds = NULL;
static u8_t *my_spiffs_cache = NULL;

static struct mtx vfs_mtx;
static struct mtx ll_mtx;

static void dir_path(char *npath, uint8_t base) {
    int len = strlen(npath);

    if (base) {
        char *c;

        c = &npath[len - 1];
        while (c >= npath) {
            if (*c == '/') {
                break;
            }

            len--;
            c--;
        }
    }

    if (len > 1) {
        if (npath[len - 1] == '.') {
            npath[len - 1] = '\0';

            if (npath[len - 2] == '/') {
                npath[len - 2] = '\0';
            }
        } else {
            if (npath[len - 1] == '/') {
                npath[len - 1] = '\0';
            }
        }
    } else {
        if ((npath[len - 1] == '/') || (npath[len - 1] == '.')) {
            npath[len - 1] = '\0';
        }
    }

    strlcat(npath, "/.", PATH_MAX);
}

static void check_path(const char *path, uint8_t *base_is_dir,
        uint8_t *full_is_dir, uint8_t *is_file, int *filenum) {
    char bpath[PATH_MAX + 1]; // Base path
    char fpath[PATH_MAX + 1]; // Full path
    struct spiffs_dirent e;
    spiffs_DIR d;
    int file_num = 0;

    *filenum = 0;
    *base_is_dir = 0;
    *full_is_dir = 0;
    *is_file = 0;

    // Get base directory name
    strlcpy(bpath, path, PATH_MAX);
    dir_path(bpath, 1);

    // Get full directory name
    strlcpy(fpath, path, PATH_MAX);
    dir_path(fpath, 0);

    SPIFFS_opendir(&fs, "/", &d);
    while (SPIFFS_readdir(&d, &e)) {
        if (!strcmp(bpath, (const char *) e.name)) {
            *base_is_dir = 1;
        }

        if (!strcmp(fpath, (const char *) e.name)) {
            *full_is_dir = 1;
        }

        if (!strcmp(path, (const char *) e.name)) {
            *is_file = 1;
        }

        if (!strncmp(fpath, (const char *) e.name, min(strlen((char * )e.name), strlen(fpath) - 1))) {
            if (strlen((const char *) e.name) >= strlen(fpath) && strcmp(fpath, (const char *) e.name)) {
                file_num++;
            }
        }
    }
    SPIFFS_closedir(&d);

    *filenum = file_num;
}

/*
 * This function translate error codes from SPIFFS to errno error codes
 *
 */
static int spiffs_result(int res) {
    switch(res) {
     case SPIFFS_OK :
     case SPIFFS_ERR_END_OF_OBJECT:
         return 0;
     case SPIFFS_ERR_NOT_WRITABLE:
     case SPIFFS_ERR_NOT_READABLE:
         return EACCES;
     case SPIFFS_ERR_NOT_MOUNTED :
     case SPIFFS_ERR_NOT_A_FS :
         return ENODEV;
     case SPIFFS_ERR_FULL :
         return ENOSPC;
     case SPIFFS_ERR_BAD_DESCRIPTOR :
         return EBADF;
     case SPIFFS_ERR_MOUNTED :
         return EEXIST;
     case SPIFFS_ERR_FILE_EXISTS :
         return EEXIST;
     case SPIFFS_ERR_NOT_FOUND :
     case SPIFFS_ERR_CONFLICTING_NAME:
     case SPIFFS_ERR_NOT_A_FILE :
     case SPIFFS_ERR_DELETED :
     case SPIFFS_ERR_FILE_DELETED :
         return ENOENT;
     case SPIFFS_ERR_NAME_TOO_LONG :
         return ENAMETOOLONG;
     case SPIFFS_ERR_RO_NOT_IMPL :
     case SPIFFS_ERR_RO_ABORTED_OPERATION :
         return EROFS;
     default:
         return EIO;
     }

     return ENOTSUP;
}

static int vfs_spiffs_traverse(const char *pathp, int *pis_dir, int *filenum, int *valid_prefix) {
    char path[PATH_MAX + 1];
    char current[PATH_MAX + 3]; // Current path
    char *dir;   // Current directory
    spiffs_stat stat;
    int res;

    int is_dir = 0;

    if (pis_dir) {
        *pis_dir = 0;
    }

    if (filenum) {
        *filenum = 0;
    }

    if (valid_prefix) {
        *valid_prefix = 1;
    }

    if (strlen(pathp) > PATH_MAX - 3) {
        return ENAMETOOLONG;
    }

    // Copy original path, because the strtok function changes the original string
    strlcpy(path, pathp, PATH_MAX);

    // Current directory is empty
    strcpy(current, "");

    // Get the first directory in path
    dir = strtok((char *)path, "/");
    while (dir) {
        // Append current directory to the current path
        strncat(current, "/", PATH_MAX);
        strncat(current, dir, PATH_MAX);

        // To check if the current path is a directory
        // we must append /.
        strncat(current, "/.", PATH_MAX);

        res = SPIFFS_stat(&fs, current, &stat);

        // Remove /. from the current path to check if the current path
        // corresponds to a file in case that is required later
        *(current + strlen(current) - 2) = '\0';

        // Get next directory in path
        dir = strtok(NULL, "/");

        if (res != SPIFFS_OK) {
            // Current path is not a directory, then check if it is a file
            res = SPIFFS_stat(&fs, current, &stat);
            if (res != SPIFFS_OK) {
                // Current path is not a directory, and it is not a file.
                if (dir) {
                    if (valid_prefix) {
                        *valid_prefix = 0;
                    }
                }

                // Error: A component in pathname does not exist
                return ENOENT;
            } else {
                // Current path is a file
                if (dir) {
                    // There are more directories in path to check.

                    if (valid_prefix) {
                        *valid_prefix = 0;
                    }

                    // Error: a component used as a directory in pathname is not,
                    // in fact, a directory
                    return ENOTDIR;
                }

                is_dir = 0;
            }
        } else {
            // Current path is a directory
            is_dir = 1;
        }
    }

    if (is_dir && filenum) {
        // Count files in directory
        struct spiffs_dirent e;
        spiffs_DIR d;

        // Get full directory name
        char fpath[PATH_MAX + 1];

        strlcpy(fpath, pathp, PATH_MAX);
        dir_path(fpath, 0);

        SPIFFS_opendir(&fs, "/", &d);
        while (SPIFFS_readdir(&d, &e)) {
            if (!strncmp(fpath, (const char *) e.name, min(strlen((char * )e.name), strlen(fpath) - 1))) {
                if (strlen((const char *) e.name) >= strlen(fpath) && strcmp(fpath, (const char *) e.name)) {
                    *filenum = *filenum + 1;
                }
            }
        }
        SPIFFS_closedir(&d);
    }

    if (pis_dir) {
        *pis_dir = (is_dir || (strcmp(pathp, "/") == 0));
    }

    return EEXIST;
}

static int vfs_spiffs_open(const char *path, int flags, int mode) {
    char npath[PATH_MAX + 1];
    int fd, result = 0;

    // Allocate new file
    vfs_file_t *file = calloc(1, sizeof(vfs_file_t));
    if (!file) {
        errno = ENOMEM;
        return -1;
    }

    file->fs_file = (void *)calloc(1, sizeof(spiffs_file));
    if (!file->fs_file) {
        free(file);
        errno = ENOMEM;
        return -1;
    }

    // Add file to file list. List index is file descriptor.
    int res = lstadd(&files, file, &fd);
    if (res) {
        free(file->fs_file);
        free(file);
        errno = res;
        return -1;
    }

    // Open file
    spiffs_flags spiffs_flgs = 0;

    // Translate flags to SPIFFS flags
    if (flags == O_RDONLY)
        spiffs_flgs |= SPIFFS_RDONLY;

    if (flags & O_WRONLY)
        spiffs_flgs |= SPIFFS_WRONLY;

    if (flags & O_RDWR)
        spiffs_flgs = SPIFFS_RDWR;

    if (flags & O_EXCL)
        spiffs_flgs |= SPIFFS_EXCL;

    if (flags & O_CREAT)
        spiffs_flgs |= SPIFFS_CREAT;

    if (flags & O_TRUNC)
        spiffs_flgs |= SPIFFS_TRUNC;

    // Check path
    uint8_t base_is_dir = 0;
    uint8_t full_is_dir = 0;
    uint8_t is_file = 0;
    int file_num = 0;

    mtx_lock(&vfs_mtx);

    check_path(path, &base_is_dir, &full_is_dir, &is_file, &file_num);

    if (full_is_dir) {
        // We want to open a directory.
        // If in flags are set some write access mode this is an error, because we only
        // can open a directory in read mode.
        if (spiffs_flgs & (SPIFFS_WRONLY | SPIFFS_CREAT | SPIFFS_TRUNC)) {
            lstremove(&files, fd, 0);
            free(file->fs_file);
            free(file);
            mtx_unlock(&vfs_mtx);
            errno = EISDIR;
            return -1;
        }

        // Open the directory
        strlcpy(npath, path, PATH_MAX);
        dir_path((char *) npath, 0);

        // Open SPIFFS file
        *((spiffs_file *)file->fs_file) = SPIFFS_open(&fs, npath, SPIFFS_RDONLY, 0);
        if (*((spiffs_file *)file->fs_file) < 0) {
            result = spiffs_result(fs.err_code);
        }

        file->is_dir = 1;
    } else {
        if (!base_is_dir) {
            // If base path is not a directory we return an error
            lstremove(&files, fd, 0);
            free(file->fs_file);
            free(file);
            mtx_unlock(&vfs_mtx);
            errno = ENOENT;
            return -1;
        } else {
            // Open SPIFFS file
            *((spiffs_file *)file->fs_file) = SPIFFS_open(&fs, path, spiffs_flgs, 0);
            if (*((spiffs_file *)file->fs_file) < 0) {
                result = spiffs_result(fs.err_code);
            }
        }
    }

    if (result != 0) {
        lstremove(&files, fd, 0);
        free(file->fs_file);
        free(file);
        mtx_unlock(&vfs_mtx);
        errno = result;
        return -1;
    }

    mtx_unlock(&vfs_mtx);

    return fd;
}

static ssize_t vfs_spiffs_write(int fd, const void *data, size_t size) {
    vfs_file_t *file;
    int res;

    res = lstget(&files, fd, (void **) &file);
    if (res || file->is_dir) {
        errno = EBADF;
        return -1;
    }

    mtx_lock(&vfs_mtx);

    // Write SPIFFS file
    res = SPIFFS_write(&fs, *((spiffs_file *)file->fs_file), (void *) data, size);
    if (res >= 0) {
        mtx_unlock(&vfs_mtx);
        return res;
    } else {
        res = spiffs_result(fs.err_code);
        if (res != 0) {
            mtx_unlock(&vfs_mtx);
            errno = res;
            return -1;
        }
    }

    mtx_unlock(&vfs_mtx);

    return -1;
}

static ssize_t vfs_spiffs_read(int fd, void * dst, size_t size) {
    vfs_file_t *file;
    int res;

    res = lstget(&files, fd, (void **) &file);
    if (res || file->is_dir) {
        errno = EBADF;
        return -1;
    }

    mtx_lock(&vfs_mtx);

    // Read SPIFFS file
    res = SPIFFS_read(&fs, *((spiffs_file *)file->fs_file), dst, size);
    if (res >= 0) {
        mtx_unlock(&vfs_mtx);
        return res;
    } else {
        res = spiffs_result(fs.err_code);
        if (res != 0) {
            mtx_unlock(&vfs_mtx);
            errno = res;
            return -1;
        }

        // EOF
        mtx_unlock(&vfs_mtx);
        return 0;
    }

    mtx_unlock(&vfs_mtx);

    return -1;
}

static int vfs_spiffs_fstat(int fd, struct stat * st) {
    vfs_file_t *file;
    spiffs_stat stat;
    int res;

    res = lstget(&files, fd, (void **) &file);
    if (res) {
        errno = EBADF;
        return -1;
    }

    // We have not time in SPIFFS
    st->st_mtime = 0;
    st->st_atime = 0;
    st->st_ctime = 0;

    // Set block size for this file system
    st->st_blksize = CONFIG_LUA_RTOS_SPIFFS_LOG_PAGE_SIZE;

    // First test if it's a directory entry
    if (file->is_dir) {
        st->st_mode = S_IFDIR;
        st->st_size = 0;
        return 0;
    }

    mtx_lock(&vfs_mtx);

    // If is not a directory get file statistics
    res = SPIFFS_fstat(&fs, *((spiffs_file *)file->fs_file), &stat);
    if (res == SPIFFS_OK) {
        st->st_size = stat.size;
    } else {
        st->st_size = 0;
        res = spiffs_result(fs.err_code);
    }

    st->st_mode = S_IFREG;

    if (res < 0) {
        mtx_unlock(&vfs_mtx);
        errno = res;
        return -1;
    }

    mtx_unlock(&vfs_mtx);

    return 0;
}

static int vfs_spiffs_close(int fd) {
    vfs_file_t *file;
    int res;

    res = lstget(&files, fd, (void **) &file);
    if (res) {
        errno = EBADF;
        return -1;
    }

    mtx_lock(&vfs_mtx);

    res = SPIFFS_close(&fs, *((spiffs_file *)file->fs_file));
    if (res) {
        res = spiffs_result(fs.err_code);
    }

    if (res < 0) {
        mtx_unlock(&vfs_mtx);
        errno = res;
        return -1;
    }

    lstremove(&files, fd, 0);

    free(file->fs_file);
    free(file);

    mtx_unlock(&vfs_mtx);

    return 0;
}

static off_t vfs_spiffs_lseek(int fd, off_t size, int mode) {
    vfs_file_t *file;
    int res;

    res = lstget(&files, fd, (void **) &file);
    if (res) {
        errno = EBADF;
        return -1;
    }

    if (file->is_dir) {
        errno = EBADF;
        return -1;
    }

    int whence = SPIFFS_SEEK_CUR;

    switch (mode) {
    case SEEK_SET:
        whence = SPIFFS_SEEK_SET;
        break;
    case SEEK_CUR:
        whence = SPIFFS_SEEK_CUR;
        break;
    case SEEK_END:
        whence = SPIFFS_SEEK_END;
        break;
    }

    mtx_lock(&vfs_mtx);

    res = SPIFFS_lseek(&fs, *((spiffs_file *)file->fs_file), size, whence);
    if (res < 0) {
        res = spiffs_result(fs.err_code);
        mtx_unlock(&vfs_mtx);
        errno = res;
        return -1;
    }

    mtx_unlock(&vfs_mtx);

    return res;
}

static int vfs_spiffs_stat(const char * path, struct stat * st) {
    int fd;
    int res;

    mtx_lock(&vfs_mtx);

    fd = vfs_spiffs_open(path, 0, 0);
    if (fd < 0) {
        mtx_unlock(&vfs_mtx);
        errno = spiffs_result(fs.err_code);
        return -1;
    }

    res = vfs_spiffs_fstat(fd, st);
    if (fd < 0) {
        mtx_unlock(&vfs_mtx);
        errno = spiffs_result(fs.err_code);
        return -1;
    }

    vfs_spiffs_close(fd);

    mtx_unlock(&vfs_mtx);

    return res;
}

static int vfs_spiffs_access(const char *path, int amode) {
    struct stat s;

    mtx_lock(&vfs_mtx);

    if (vfs_spiffs_stat(path, &s) < 0) {
        mtx_unlock(&vfs_mtx);
        return -1;
    }

    if (s.st_mode != S_IFREG) {
        mtx_unlock(&vfs_mtx);
        errno = EACCES;
        return -1;
    }

    mtx_unlock(&vfs_mtx);

    return 0;
}

static int vfs_spiffs_unlink(const char *path) {
    int is_dir;

    mtx_lock(&vfs_mtx);

    int res = vfs_spiffs_traverse(path, &is_dir, NULL, NULL);
    if  ((res != 0) && (res != EEXIST)) {
        mtx_unlock(&vfs_mtx);
        errno = res;
        return -1;
    }

    if (is_dir) {
        mtx_unlock(&vfs_mtx);
        errno = EPERM;
        return -1;
    }

    res = SPIFFS_remove(&fs, path);
    if (res) {
        mtx_unlock(&vfs_mtx);
        errno = spiffs_result(fs.err_code);
        return -1;
    }

    mtx_unlock(&vfs_mtx);

    return 0;
}

static int vfs_spiffs_rename(const char *src, const char *dst) {
    int src_is_dir;
    int dst_is_dir;
    int dst_files;

    mtx_lock(&vfs_mtx);

    int res = vfs_spiffs_traverse(src, &src_is_dir, NULL, NULL);
    if  ((res != 0) && (res != EEXIST) && (res != ENOENT)) {
        mtx_unlock(&vfs_mtx);
        errno = res;
        return -1;
    }

    res = vfs_spiffs_traverse(dst, &dst_is_dir, &dst_files, NULL);
    if  ((res != 0) && (res != EEXIST) && (res != ENOENT)) {
        mtx_unlock(&vfs_mtx);
        errno = res;
        return -1;
    }

    if (dst_is_dir && !src_is_dir) {
        mtx_unlock(&vfs_mtx);
        errno = EISDIR;
        return -1;
    }

    if (dst_is_dir && (dst_files > 0)) {
        mtx_unlock(&vfs_mtx);
        errno = ENOTEMPTY;
        return -1;
    }

    char dpath[PATH_MAX + 1];
    char *csrc;
    char *cname;

    if (src_is_dir) {
        // We need to rename all tree
        struct spiffs_dirent e;
        spiffs_DIR d;

        SPIFFS_opendir(&fs, "/", &d);
        while (SPIFFS_readdir(&d, &e)) {
            if (!strncmp(src, (const char *) e.name, strlen(src)) && e.name[strlen(src)] == '/') {
                strlcpy(dpath, dst, PATH_MAX);
                csrc = (char *) src;
                cname = (char *) e.name;

                while (*csrc && *cname && (*csrc == *cname)) {
                    ++csrc;
                    ++cname;
                }

                strlcat(dpath, cname, PATH_MAX);

                if (SPIFFS_rename(&fs, (char *) e.name, dpath) != SPIFFS_OK) {
                    if (fs.err_code != SPIFFS_ERR_CONFLICTING_NAME) {
                        mtx_unlock(&vfs_mtx);
                        errno = spiffs_result(fs.err_code);
                        return -1;
                    } else {
                        // This only happens when e.name and dpath are directories. In this case
                        // remove e.name
                        if (SPIFFS_remove(&fs, (char *) e.name) != SPIFFS_OK) {
                            mtx_unlock(&vfs_mtx);
                            errno = spiffs_result(fs.err_code);
                            return -1;
                        }
                    }
                }
            }
        }
        SPIFFS_closedir(&d);
    } else {
        if (SPIFFS_rename(&fs, src, dst) < 0) {
            mtx_unlock(&vfs_mtx);
            errno = spiffs_result(fs.err_code);
            return -1;
        }
    }

    mtx_unlock(&vfs_mtx);

    return 0;
}

static DIR* vfs_spiffs_opendir(const char* name) {
    int is_dir;

    vfs_dir_t *dir = vfs_allocate_dir("spiffs", name);
    if (!dir) {
        return NULL;
    }

    mtx_lock(&vfs_mtx);

    int res = vfs_spiffs_traverse(name, &is_dir, NULL, NULL);
    if  ((res != 0) && (res != EEXIST)) {
        vfs_free_dir(dir);
        mtx_unlock(&vfs_mtx);
        errno = res;
        return NULL;
    }

    if ((res == EEXIST) && !is_dir) {
        vfs_free_dir(dir);
        mtx_unlock(&vfs_mtx);
        errno = ENOTDIR;
        return NULL;
    }

    if (!is_dir) {
        vfs_free_dir(dir);
        mtx_unlock(&vfs_mtx);
        errno = ENOENT;
        return NULL;
    }

    if (!SPIFFS_opendir(&fs, name, (spiffs_DIR *)dir->fs_dir)) {
        vfs_free_dir(dir);
        mtx_unlock(&vfs_mtx);
        errno = spiffs_result(fs.err_code);
        return NULL;
    }

    mtx_unlock(&vfs_mtx);

    return (DIR *) dir;
}

static int vfs_spiffs_rmdir(const char *path) {
    int is_dir;
    int filenum;

    if (strcmp(path,"/") == 0) {
        errno = EBUSY;
        return -1;
    }

    mtx_lock(&vfs_mtx);

    int res = vfs_spiffs_traverse(path, &is_dir, &filenum, NULL);
    if  ((res != 0) && (res != EEXIST)) {
        mtx_unlock(&vfs_mtx);
        errno = res;
        return -1;
    }

    if (!is_dir) {
        mtx_unlock(&vfs_mtx);
        errno = ENOTDIR;
        return -1;
    }

    if (filenum > 0) {
        mtx_unlock(&vfs_mtx);
        errno = ENOTEMPTY;
        return -1;
    }

    // Remove directory
    char npath[PATH_MAX + 1];

    strlcpy(npath, path, PATH_MAX);
    dir_path(npath, 0);

    res = SPIFFS_remove(&fs, npath);
    if (res) {
        mtx_unlock(&vfs_mtx);
        errno = spiffs_result(fs.err_code);
        return -1;
    }

    mtx_unlock(&vfs_mtx);

    return 0;
}

static struct dirent* vfs_spiffs_readdir(DIR* pdir) {
    int res = 0, len = 0, entries = 0;
    vfs_dir_t *dir = (vfs_dir_t *) pdir;

    struct dirent *ent = &dir->ent;

    char *fn;

    // Clear the current dirent
    memset(ent, 0, sizeof(struct dirent));

    // If there are mount points to read, read them first
    if (dir->mount) {
        struct dirent *ment = mount_readdir((DIR *)dir);
        if (ment) {
            return ment;
        }
    }

    mtx_lock(&vfs_mtx);

    // Search for next entry
    for (;;) {
        // Read directory
        dir->fs_info = (void *)SPIFFS_readdir((spiffs_DIR *)dir->fs_dir, (struct spiffs_dirent *)dir->fs_info);
        if (!dir->fs_info) {
            if (fs.err_code != SPIFFS_VIS_END) {
                res = spiffs_result(fs.err_code);
                errno = res;
            }

            break;
        }

        // Break condition
        if (((struct spiffs_dirent *)(dir->fs_info))->name[0] == 0)
            break;

        // Get name and length
        fn = (char *)(((struct spiffs_dirent *)(dir->fs_info))->name);
        len = strlen(fn);

        // Get entry type and size
        ent->d_type = DT_REG;

        if (len >= 2) {
            if (fn[len - 1] == '.') {
                if (fn[len - 2] == '/') {
                    ent->d_type = DT_DIR;

                    fn[len - 2] = '\0';

                    len = strlen(fn);

                    // Skip root dir
                    if (len == 0) {
                        continue;
                    }
                }
            }
        }

        // Skip entries not belonged to path
        if (strncmp(fn, dir->path, strlen(dir->path)) != 0) {
            continue;
        }

        if (strlen(dir->path) > 1) {
            if (*(fn + strlen(dir->path)) != '/') {
                continue;
            }
        }

        // Skip root directory
        fn = fn + strlen(dir->path);
        len = strlen(fn);
        if (len == 0) {
            continue;
        }

        // Skip initial /
        if (len > 1) {
            if (*fn == '/') {
                fn = fn + 1;
                len--;
            }
        }

        // Skip subdirectories
        if (strchr(fn, '/')) {
            continue;
        }

        ent->d_fsize = ((struct spiffs_dirent *)(dir->fs_info))->size;

        strlcpy(ent->d_name, fn, MAXNAMLEN);

        entries++;
        dir->offset++;

        break;
    }

    mtx_unlock(&vfs_mtx);

    if (entries > 0) {
        return ent;
    } else {
        return NULL;
    }
}

static long vfs_spiffs_telldir(DIR *dirp) {
    vfs_dir_t *dir = (vfs_dir_t *)dirp;

    if (dir->offset < 0) {
    		errno = EBADF;
    		return -1;
    }

    return (long)dir->offset;
}

static int vfs_spiffs_closedir(DIR* pdir) {
    vfs_dir_t *dir = (vfs_dir_t *) pdir;
    int res;

    if (!pdir) {
        errno = EBADF;
        return -1;
    }

    mtx_lock(&vfs_mtx);

    if ((res = SPIFFS_closedir((spiffs_DIR *)dir->fs_dir)) < 0) {
        mtx_unlock(&vfs_mtx);
        errno = spiffs_result(fs.err_code);
        return -1;
    }

    vfs_free_dir(dir);

    mtx_unlock(&vfs_mtx);

    return 0;
}

static int vfs_spiffs_mkdir(const char *path, mode_t mode) {
    char npath[PATH_MAX + 1];
    int res;
    int valid_prefix;

    mtx_lock(&vfs_mtx);

    res = vfs_spiffs_traverse(path, NULL, NULL, &valid_prefix);
    if  ((res != 0) && ((res != ENOENT) || (!valid_prefix))) {
        mtx_unlock(&vfs_mtx);
        errno = res;
        return -1;
    }

    // Create directory
    strlcpy(npath, path, PATH_MAX);
    dir_path(npath, 0);

    spiffs_file fd = SPIFFS_open(&fs, npath, SPIFFS_CREAT | SPIFFS_RDWR, 0);
    if (fd < 0) {
        mtx_unlock(&vfs_mtx);
        res = spiffs_result(fs.err_code);
        errno = res;
        return -1;
    }

    if (SPIFFS_close(&fs, fd) < 0) {
        mtx_unlock(&vfs_mtx);
        res = spiffs_result(fs.err_code);
        errno = res;
        return -1;
    }

    mtx_unlock(&vfs_mtx);

    return 0;
}

static int vfs_spiffs_fsync(int fd) {
    vfs_file_t *file;
    int res;

    res = lstget(&files, fd, (void **) &file);
    if (res) {
        errno = EBADF;
        return -1;
    }

    if (file->is_dir) {
        errno = EBADF;
        return -1;
    }

    mtx_lock(&vfs_mtx);

    res = SPIFFS_fflush(&fs, *((spiffs_file *)file->fs_file));
    if (res >= 0) {
        mtx_unlock(&vfs_mtx);
        return res;
    } else {
        res = spiffs_result(fs.err_code);
        if (res != 0) {
            mtx_unlock(&vfs_mtx);
            errno = res;
            return -1;
        }

        mtx_unlock(&vfs_mtx);
        errno = EIO;
        return -1;
    }

    mtx_unlock(&vfs_mtx);

    return 0;
}

static void vfs_spiffs_free_resources() {
    if (my_spiffs_work_buf) free(my_spiffs_work_buf);
    if (my_spiffs_fds) free(my_spiffs_fds);
    if (my_spiffs_cache) free(my_spiffs_cache);

    mtx_destroy(&vfs_mtx);
    mtx_destroy(&ll_mtx);
}

int vfs_spiffs_mount(const char *target) {
    esp_vfs_t vfs = {
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_spiffs_write,
        .open = &vfs_spiffs_open,
        .fstat = &vfs_spiffs_fstat,
        .close = &vfs_spiffs_close,
        .read = &vfs_spiffs_read,
        .lseek = &vfs_spiffs_lseek,
        .stat = &vfs_spiffs_stat,
        .link = NULL,
        .unlink = &vfs_spiffs_unlink,
        .rename = &vfs_spiffs_rename,
        .mkdir = &vfs_spiffs_mkdir,
        .opendir = &vfs_spiffs_opendir,
        .readdir = &vfs_spiffs_readdir,
        .closedir = &vfs_spiffs_closedir,
        .rmdir = &vfs_spiffs_rmdir,
        .fsync = &vfs_spiffs_fsync,
        .access = &vfs_spiffs_access,
		.telldir = &vfs_spiffs_telldir,
    };

    // Mount spiffs file system
    spiffs_config cfg;
    int res = 0;
    int retries = 0;

    // Find partition
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, LUA_RTOS_SPIFFS_PART, NULL);

    if (!partition) {
        vfs_spiffs_free_resources();
        syslog(LOG_ERR, "spiffs can't find a valid partition");
        return -1;
    } else {
        cfg.phys_addr = partition->address;
        cfg.phys_size = partition->size;
    }

    cfg.phys_erase_block = CONFIG_LUA_RTOS_SPIFFS_ERASE_SIZE;
    cfg.log_page_size = CONFIG_LUA_RTOS_SPIFFS_LOG_PAGE_SIZE;
    cfg.log_block_size = CONFIG_LUA_RTOS_SPIFFS_LOG_BLOCK_SIZE;

    if (partition) {
        syslog(LOG_INFO,
                "spiffs start address at 0x%x, size %d Kb",
                cfg.phys_addr, cfg.phys_size / 1024);
    } else {
        syslog(LOG_INFO, "spiffs start address at 0x%x, size %d Kb",
                cfg.phys_addr, cfg.phys_size / 1024);
    }

    cfg.hal_read_f  = (spiffs_read)  low_spiffs_read;
    cfg.hal_write_f = (spiffs_write) low_spiffs_write;
    cfg.hal_erase_f = (spiffs_erase) low_spiffs_erase;

    my_spiffs_work_buf = malloc(cfg.log_page_size * 2);
    if (!my_spiffs_work_buf) {
        vfs_spiffs_free_resources();
        syslog(LOG_ERR, "spiffs can't allocate memory for file system");
        return -1;
    }

    int fds_len = sizeof(spiffs_fd) * 5;
    my_spiffs_fds = malloc(fds_len);
    if (!my_spiffs_fds) {
        vfs_spiffs_free_resources();
        syslog(LOG_ERR, "spiffs can't allocate memory for file system");
        return -1;
    }

    int cache_len = cfg.log_page_size * 5;
    my_spiffs_cache = malloc(cache_len);
    if (!my_spiffs_cache) {
        vfs_spiffs_free_resources();
        syslog(LOG_ERR, "spiffs can't allocate memory for file system");
        return -1;
    }

    // Init mutex
    mtx_init(&vfs_mtx, NULL, NULL, MTX_RECURSE);

    mtx_init(&ll_mtx, NULL, NULL, 0);
    fs.user_data = &ll_mtx;

    while (retries < 2) {
        res = SPIFFS_mount(&fs, &cfg, my_spiffs_work_buf, my_spiffs_fds,
                fds_len, my_spiffs_cache, cache_len, NULL);

        if (res < 0) {
            if (fs.err_code == SPIFFS_ERR_NOT_A_FS) {
                syslog(LOG_ERR, "spiffs no file system detected, formating...");
                SPIFFS_unmount(&fs);
                res = SPIFFS_format(&fs);
                if (res < 0) {
                    vfs_spiffs_free_resources();
                    syslog(LOG_ERR, "spiffs format error");
                    return -1;
                }
            } else {
                vfs_spiffs_free_resources();
                syslog(LOG_ERR, "spiff can't mount file system (%s)",
                        strerror(spiffs_result(fs.err_code)));
                return -1;
            }
        } else {
            break;
        }

        retries++;
    }

    if (retries > 0) {
        syslog(LOG_INFO, "spiffs creating root folder");

        // Create the root folder
        spiffs_file fd = SPIFFS_open(&fs, "/.", SPIFFS_CREAT | SPIFFS_RDWR, 0);
        if (fd < 0) {
            vfs_spiffs_umount(target);
            syslog(LOG_ERR, "spiffs can't create root folder (%s)",
                    strerror(spiffs_result(fs.err_code)));
            return -1;
        }

        if (SPIFFS_close(&fs, fd) < 0) {
            vfs_spiffs_umount(target);
            syslog(LOG_ERR, "spiffs can't create root folder (%s)",
                    strerror(spiffs_result(fs.err_code)));
            return -1;
        }
    }

    lstinit(&files, 0, LIST_DEFAULT);

    ESP_ERROR_CHECK(esp_vfs_register("/spiffs", &vfs, NULL));

    syslog(LOG_INFO, "spiffs mounted on %s", target);

    return 0;
}

int vfs_spiffs_umount(const char *target) {
    esp_vfs_unregister("/spiffs");
    SPIFFS_unmount(&fs);

    vfs_spiffs_free_resources();

    syslog(LOG_INFO, "spiffs unmounted");

    return 0;
}

int vfs_spiffs_format(const char *target) {
    vfs_spiffs_umount(target);

    mtx_init(&ll_mtx, NULL, NULL, 0);

    int res = SPIFFS_format(&fs);
    if (res < 0) {
        syslog(LOG_ERR, "spiffs format error");
        return -1;
    }

    mtx_destroy(&ll_mtx);

    vfs_spiffs_mount(target);

    syslog(LOG_INFO, "spiffs creating root folder");

    // Create the root folder
    spiffs_file fd = SPIFFS_open(&fs, "/.", SPIFFS_CREAT | SPIFFS_RDWR, 0);
    if (fd < 0) {
        vfs_spiffs_umount(target);
        syslog(LOG_ERR, "spiffs can't create root folder (%s)",
                strerror(spiffs_result(fs.err_code)));
        return -1;
    }

    if (SPIFFS_close(&fs, fd) < 0) {
        vfs_spiffs_umount(target);
        syslog(LOG_ERR, "spiffs can't create root folder (%s)",
                strerror(spiffs_result(fs.err_code)));
        return -1;
    }

    return 0;
}

#endif

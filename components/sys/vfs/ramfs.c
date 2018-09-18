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
 * Lua RTOS, RAM file system
 *
 */

#include "sdkconfig.h"

#include "esp_partition.h"

#include <freertos/FreeRTOS.h>

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <dirent.h>

#include <sys/stat.h>

#include "esp_vfs.h"

#include <errno.h>

#include "ramfs.h"

#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/mutex.h>
#include <sys/list.h>
#include <sys/fcntl.h>
#include <sys/vfs/vfs.h>
#include <dirent.h>

static int vfs_ramfs_open(const char *path, int flags, int mode);
static ssize_t vfs_ramfs_write(int fd, const void *data, size_t size);
static ssize_t vfs_ramfs_read(int fd, void * dst, size_t size);
static int vfs_ramfs_fstat(int fd, struct stat * st);
static int vfs_ramfs_close(int fd);
static off_t vfs_ramfs_lseek(int fd, off_t size, int mode);
static int vfs_ramfs_access(const char *path, int amode);

static struct list files;
static ramfs_t fs;

/*
 * This function translate file system errors to POSIX errors
 *
 */
static int ramfs_to_errno(int res) {
    switch (res) {
    case RAMFS_ERR_OK:
        return 0;
    case RAMFS_ERR_NOMEM:
        return ENOMEM;
    case RAMFS_ERR_NOENT:
        return ENOENT;
    case RAMFS_ERR_EXIST:
        return EEXIST;
    case RAMFS_ERR_NOTDIR:
        return ENOTDIR;
    case RAMFS_ERR_BADF:
        return EBADF;
    case RAMFS_ERR_ACCESS:
        return EACCES;
    case RAMFS_ERR_NOSPC:
        return ENOSPC;
    case RAMFS_ERR_INVAL:
        return EINVAL;
    case RAMFS_ERR_ISDIR:
        return EISDIR;
    case RAMFS_ERR_NOTEMPTY:
        return ENOTEMPTY;
    case RAMFS_ERR_BUSY:
        return EBUSY;
    case RAMFS_ERR_PERM:
        return EPERM;
    case RAMFS_ERR_NAMETOOLONG:
        return ENAMETOOLONG;
    }

    return ENOTSUP;
}

static int vfs_ramfs_open(const char *path, int flags, int mode) {
    int fd;
    int result;

    // Allocate new file
    vfs_file_t *file = calloc(1, sizeof(vfs_file_t));
    if (!file) {
        errno = ENOMEM;
        return -1;
    }

    file->fs_file = (void *)calloc(1, sizeof(ramfs_file_t));
    if (!file->fs_file) {
        free(file);
        errno = ENOMEM;
        return -1;
    }

    // Add file to file list and get the file descriptor
    int res = lstadd(&files, file, &fd);
    if (res) {
        free(file->fs_file);
        free(file);
        errno = res;
        return -1;
    }

    // Translate POSIX flags to file system flags
    int ramfs_flags = 0;

    // Access mode
    int access_mode = flags & O_ACCMODE;

    if (access_mode == O_RDONLY) {
        ramfs_flags |= RAMFS_O_RDONLY;
    } else if (access_mode == O_WRONLY) {
        ramfs_flags |= RAMFS_O_WRONLY;
    } else if (access_mode == O_RDWR) {
        ramfs_flags |= RAMFS_O_RDWR;
    }

    // File status
    if (flags & O_CREAT) {
        ramfs_flags |= RAMFS_O_CREAT;
    }

    if (flags & O_EXCL) {
        ramfs_flags |= RAMFS_O_EXCL;
    }

    if (flags & O_TRUNC) {
        ramfs_flags |= RAMFS_O_TRUNC;
    }

    if (flags & O_APPEND) {
        ramfs_flags |= RAMFS_O_APPEND;
    }

    if ((result = ramfs_file_open(&fs, file->fs_file, path, ramfs_flags)) != RAMFS_ERR_OK) {
        errno = ramfs_to_errno(result);
        lstremove(&files, fd, 0);

        free(file->fs_file);
        free(file);

        return -1;
    }

    return fd;
}

static ssize_t vfs_ramfs_write(int fd, const void *data, size_t size) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    // Write to file
    result = ramfs_file_write(&fs, file->fs_file, (void *)data, size);
    if (result < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    return result;
}

static ssize_t vfs_ramfs_read(int fd, void *dst, size_t size) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    // Read from file
    result = ramfs_file_read(&fs, file->fs_file, dst, size);
    if (result < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    return result;
}

static int vfs_ramfs_fstat(int fd, struct stat *st) {
    vfs_file_t *file;
    ramfs_info_t info;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    // Init stats
    memset(st, 0, sizeof(struct stat));

    // Get the file stats
    result = ramfs_file_stat(&fs, file->fs_file, &info);
    if (result < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    st->st_size = info.size;
    st->st_mode = ((info.type==RAMFS_FILE)?S_IFREG:S_IFDIR);
    st->st_blksize = fs.block_size;

    return 0;
}

static int vfs_ramfs_close(int fd) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    // Close file
    result = ramfs_file_close(&fs, file->fs_file);
    if (result < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    // Remove file from file list
    lstremove(&files, fd, 0);

    free(file->fs_file);
    free(file);

    return 0;
}

static off_t vfs_ramfs_lseek(int fd, off_t size, int mode) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    // Convert POSIX whence to file system whence
    int whence = RAMFS_SEEK_CUR;

    switch (mode) {
    case SEEK_SET:
        whence = RAMFS_SEEK_SET;
        break;
    case SEEK_CUR:
        whence = RAMFS_SEEK_CUR;
        break;
    case SEEK_END:
        whence = RAMFS_SEEK_END;
        break;
    default:
        errno = EINVAL;
        return -1;
    }

    result = ramfs_file_seek(&fs, file->fs_file, size, whence);
    if (result < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    return result;
}

static int vfs_ramfs_stat(const char *path, struct stat *st) {
    ramfs_info_t info;
    int result;

    // Init stats
    memset(st, 0, sizeof(struct stat));

    // Get the file stats
    result = ramfs_stat(&fs, path, &info);
    if (result < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    st->st_size = info.size;
    st->st_mode = ((info.type==RAMFS_FILE)?S_IFREG:S_IFDIR);
    st->st_blksize = fs.block_size;

    return 0;
}

static int vfs_ramfs_access(const char *path, int amode) {
#if 0
    struct stat s;

    if (vfs_ramfs_stat(path, &s) < 0) {
        return -1;
    }

    if (s.st_mode != S_IFREG) {
        errno = EACCES;
        return -1;
    }

    return 0;
#endif
    return 0;

}

static int vfs_ramfs_unlink(const char *path) {
    int result;

    if ((result = ramfs_unlink(&fs, path)) < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    return 0;
}

static int vfs_ramfs_rename(const char *src, const char *dst) {
    int result;

    if ((result = ramfs_rename(&fs, src, dst)) < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    return 0;
}

static DIR* vfs_ramfs_opendir(const char *name) {
    int result;

    vfs_dir_t *dir = vfs_allocate_dir("rfs", name);
    if (!dir) {
        return NULL;
    }

    // Open directory
    if ((result = ramfs_dir_open(&fs, dir->fs_dir, name)) < 0) {
        errno = ramfs_to_errno(result);
        vfs_free_dir(dir);

        return NULL;
    }

    return (DIR *)dir;
}

static int vfs_ramfs_rmdir(const char *name) {
    int result;

    if ((result = ramfs_rmdir(&fs, name)) < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    return 0;
}

static struct dirent *vfs_ramfs_readdir(DIR *pdir) {
    vfs_dir_t *dir = (vfs_dir_t *)pdir;
    struct dirent *ent = &dir->ent;
    int result;

    // Clear current entry
    memset(ent, 0, sizeof(struct dirent));

    // If there are mount points to read, read them first
    if (dir->mount) {
        struct dirent *ment = mount_readdir((DIR *)dir);
        if (ment) {
            return ment;
        }
    }

    // Read next directory entry
    if ((result = ramfs_dir_read(&fs, ((vfs_dir_t *)pdir)->fs_dir, (ramfs_info_t *)dir->fs_info)) == RAMFS_ERR_OK) {
        ent->d_type = (((ramfs_info_t *)dir->fs_info)->type == RAMFS_FILE)?DT_REG:DT_DIR;
        ent->d_fsize = ((ramfs_info_t *)dir->fs_info)->size;
        strlcpy(ent->d_name, ((ramfs_info_t *)dir->fs_info)->name, MAXNAMLEN);

        return ent;
    }

    if (result != RAMFS_ERR_NOENT) {
        errno = ramfs_to_errno(result);
    }

    return NULL;
}

static long vfs_ramfs_telldir(DIR *dirp) {
    vfs_dir_t *dir = (vfs_dir_t *)dirp;
    ramfs_off_t offset;

    offset = ramfs_telldir(&fs, dir->fs_dir);
    if (offset < 0) {
        errno = ramfs_to_errno(offset);
        return -1;
    }

    return (long)offset;
}

static int vfs_ramfs_closedir(DIR *pdir) {
    vfs_dir_t *dir = ((vfs_dir_t *)pdir);
    int result;

    if ((result = ramfs_dir_close(&fs, dir->fs_dir)) < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    vfs_free_dir(dir);

    return 0;
}

static int vfs_ramfs_mkdir(const char *path, mode_t mode) {
    int result;

    if ((result = ramfs_mkdir(&fs, path)) < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    return 0;
}

static int vfs_ramfs_fsync(int fd) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    result = ramfs_file_sync(&fs, file->fs_file);
    if (result < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    return 0;
}

static int vfs_ramfs_ftruncate(int fd, off_t length) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    if ((result = ramfs_file_truncate(&fs, file->fs_file, length)) != RAMFS_ERR_OK) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    return 0;
}

static int vfs_ramfs_truncate(const char *path, off_t length) {
    ramfs_file_t file;
    int result;

    if ((result = ramfs_file_open(&fs, &file, path, RAMFS_O_RDWR)) != RAMFS_ERR_OK) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    if ((result = ramfs_file_truncate(&fs, &file, length)) != RAMFS_ERR_OK) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    result = ramfs_file_close(&fs, &file);
    if (result < 0) {
        errno = ramfs_to_errno(result);
        return -1;
    }

    return 0;
}

void vfs_ramfs_mount() {
    esp_vfs_t vfs = {
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_ramfs_write,
        .open = &vfs_ramfs_open,
        .fstat = &vfs_ramfs_fstat,
        .close = &vfs_ramfs_close,
        .read = &vfs_ramfs_read,
        .lseek = &vfs_ramfs_lseek,
        .stat = &vfs_ramfs_stat,
        .link = NULL,
        .unlink = &vfs_ramfs_unlink,
        .rename = &vfs_ramfs_rename,
        .mkdir = &vfs_ramfs_mkdir,
        .opendir = &vfs_ramfs_opendir,
        .readdir = &vfs_ramfs_readdir,
        .closedir = &vfs_ramfs_closedir,
        .rmdir = &vfs_ramfs_rmdir,
        .fsync = &vfs_ramfs_fsync,
        .access = &vfs_ramfs_access,
        .telldir = &vfs_ramfs_telldir,
        .truncate = &vfs_ramfs_truncate,
        .ftruncate = &vfs_ramfs_ftruncate,
    };

    // Register the file system
    ESP_ERROR_CHECK(esp_vfs_register("/rfs", &vfs, NULL));

    int res;
    ramfs_config_t config;

    config.size = CONFIG_LUA_RTOS_RAM_FS_SIZE;
    config.block_size = CONFIG_LUA_RTOS_RAM_FS_BLOCK_SIZE;

    syslog(LOG_INFO, "ramfs size %d Kb, block size %d bytes", config.size / 1024, config.block_size);

    res = ramfs_mount(&fs, &config);

    mount_set_mounted("rfs", (res == RAMFS_ERR_OK));

    if (res == RAMFS_ERR_OK) {
        lstinit(&files, 0, LIST_DEFAULT);

        syslog(LOG_INFO, "ramfs mounted on %s", mount_get_mount_point_for_fs("rfs")->fpath);
    } else {
        syslog(LOG_INFO, "rfs mount error");
    }
}

void vfs_ramfs_umount() {
	ramfs_umount(&fs);
	esp_vfs_unregister("/rfs");
    mount_set_mounted("rfs", 0);

    syslog(LOG_INFO, "rfs unmounted");
}

void vfs_ramfs_format() {
    vfs_ramfs_umount();
    vfs_ramfs_mount();
}

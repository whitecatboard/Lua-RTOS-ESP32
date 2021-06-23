/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, ROM file system
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_ROM_FS

#include <freertos/FreeRTOS.h>

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <dirent.h>

#include <sys/stat.h>

#include "esp_vfs.h"

#include <errno.h>

#include "romfs.h"

#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/mutex.h>
#include <sys/list.h>
#include <sys/fcntl.h>
#include <sys/vfs/vfs.h>
#include <dirent.h>

static int vfs_romfs_open(const char *path, int flags, int mode);
static ssize_t vfs_romfs_write(int fd, const void *data, size_t size);
static ssize_t vfs_romfs_read(int fd, void * dst, size_t size);
static int vfs_romfs_fstat(int fd, struct stat * st);
static int vfs_romfs_close(int fd);
static off_t vfs_romfs_lseek(int fd, off_t size, int mode);
static int vfs_romfs_access(const char *path, int amode);

static struct list files;
static romfs_t fs;

extern uint8_t _romfs_start[];

/*
 * This function translate file system errors to POSIX errors
 *
 */
static int romfs_to_errno(int res) {
    switch (res) {
    case ROMFS_ERR_OK:
        return 0;
    case ROMFS_ERR_NOMEM:
        return ENOMEM;
    case ROMFS_ERR_NOENT:
        return ENOENT;
    case ROMFS_ERR_EXIST:
        return EEXIST;
    case ROMFS_ERR_NOTDIR:
        return ENOTDIR;
    case ROMFS_ERR_BADF:
        return EBADF;
    case ROMFS_ERR_ACCESS:
        return EACCES;
    case ROMFS_ERR_NOSPC:
        return ENOSPC;
    case ROMFS_ERR_INVAL:
        return EINVAL;
    case ROMFS_ERR_ISDIR:
        return EISDIR;
    case ROMFS_ERR_NOTEMPTY:
        return ENOTEMPTY;
    case ROMFS_ERR_BUSY:
        return EBUSY;
    case ROMFS_ERR_PERM:
        return EPERM;
    case ROMFS_ERR_NAMETOOLONG:
        return ENAMETOOLONG;
    }

    return ENOTSUP;
}

static int vfs_romfs_open(const char *path, int flags, int mode) {
    int fd;
    int result;

    // Allocate new file
    vfs_file_t *file = calloc(1, sizeof(vfs_file_t));
    if (!file) {
        errno = ENOMEM;
        return -1;
    }

    file->fs_file = (void *)calloc(1, sizeof(romfs_file_t));
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
    int romfs_flags = 0;

    // Access mode
    int access_mode = flags & O_ACCMODE;

    if (access_mode == O_RDONLY) {
        romfs_flags |= ROMFS_O_RDONLY;
    } else if (access_mode == O_WRONLY) {
        free(file->fs_file);
        free(file);
    		errno = EROFS;
    		return -1;
    } else if (access_mode == O_RDWR) {
        free(file->fs_file);
        free(file);
		errno = EROFS;
		return -1;
    }

    // File status
    if (flags & O_CREAT) {
        free(file->fs_file);
        free(file);
		errno = EROFS;
		return -1;
    }

    if (flags & O_EXCL) {
        free(file->fs_file);
        free(file);
		errno = EROFS;
		return -1;
    }

    if (flags & O_TRUNC) {
        free(file->fs_file);
        free(file);
		errno = EROFS;
		return -1;
    }

    if (flags & O_APPEND) {
        free(file->fs_file);
        free(file);
		errno = EROFS;
		return -1;
    }

    if ((result = romfs_file_open(&fs, file->fs_file, path, romfs_flags)) != ROMFS_ERR_OK) {
        errno = romfs_to_errno(result);
        lstremove(&files, fd, 0);

        free(file->fs_file);
        free(file);

        return -1;
    }

    return fd;
}

static ssize_t vfs_romfs_write(int fd, const void *data, size_t size) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    errno = EROFS;
    return -1;
}

static ssize_t vfs_romfs_read(int fd, void *dst, size_t size) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    // Read from file
    result = romfs_file_read(&fs, file->fs_file, dst, size);
    if (result < 0) {
        errno = romfs_to_errno(result);
        return -1;
    }

    return result;
}

static int vfs_romfs_fstat(int fd, struct stat *st) {
    vfs_file_t *file;
    romfs_info_t info;
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
    result = romfs_file_stat(&fs, file->fs_file, &info);
    if (result < 0) {
        errno = romfs_to_errno(result);
        return -1;
    }

    st->st_size = info.size;
    st->st_mode = ((info.type==ROMFS_FILE)?S_IFREG:S_IFDIR);
    st->st_blksize = 512;

    return 0;
}

static int vfs_romfs_close(int fd) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    // Close file
    result = romfs_file_close(&fs, file->fs_file);
    if (result < 0) {
        errno = romfs_to_errno(result);
        return -1;
    }

    // Remove file from file list
    lstremove(&files, fd, 0);

    free(file->fs_file);
    free(file);

    return 0;
}

static off_t vfs_romfs_lseek(int fd, off_t size, int mode) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    // Convert POSIX whence to file system whence
    int whence = ROMFS_SEEK_CUR;

    switch (mode) {
    case SEEK_SET:
        whence = ROMFS_SEEK_SET;
        break;
    case SEEK_CUR:
        whence = ROMFS_SEEK_CUR;
        break;
    case SEEK_END:
        whence = ROMFS_SEEK_END;
        break;
    default:
        errno = EINVAL;
        return -1;
    }

    result = romfs_file_seek(&fs, file->fs_file, size, whence);
    if (result < 0) {
        errno = romfs_to_errno(result);
        return -1;
    }

    return result;
}

static int vfs_romfs_stat(const char *path, struct stat *st) {
    romfs_info_t info;
    int result;

    // Init stats
    memset(st, 0, sizeof(struct stat));

    // Get the file stats
    result = romfs_stat(&fs, path, &info);
    if (result < 0) {
        errno = romfs_to_errno(result);
        return -1;
    }

    st->st_size = info.size;
    st->st_mode = ((info.type==ROMFS_FILE)?S_IFREG:S_IFDIR);
    st->st_blksize = 512;

    return 0;
}

static int vfs_romfs_access(const char *path, int amode) {
#if 0
    struct stat s;

    if (vfs_romfs_stat(path, &s) < 0) {
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

static int vfs_romfs_unlink(const char *path) {
    errno = EROFS;
    return -1;
}

static int vfs_romfs_rename(const char *src, const char *dst) {
    errno = EROFS;
    return -1;
}

static DIR* vfs_romfs_opendir(const char *name) {
    int result;

    vfs_dir_t *dir = vfs_allocate_dir("romfs", name);
    if (!dir) {
        return NULL;
    }

    // Open directory
    if ((result = romfs_dir_open(&fs, dir->fs_dir, name)) < 0) {
        errno = romfs_to_errno(result);
        vfs_free_dir(dir);

        return NULL;
    }

    return (DIR *)dir;
}

static int vfs_romfs_rmdir(const char *name) {
    errno = EROFS;
    return -1;
}

static struct dirent *vfs_romfs_readdir(DIR *pdir) {
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
    if ((result = romfs_dir_read(&fs, ((vfs_dir_t *)pdir)->fs_dir, (romfs_info_t *)dir->fs_info)) == ROMFS_ERR_OK) {
        ent->d_type = (((romfs_info_t *)dir->fs_info)->type == ROMFS_FILE)?DT_REG:DT_DIR;
        ent->d_fsize = ((romfs_info_t *)dir->fs_info)->size;
        strlcpy(ent->d_name, ((romfs_info_t *)dir->fs_info)->name, MAXNAMLEN);

        return ent;
    }

    if (result != ROMFS_ERR_NOENT) {
        errno = romfs_to_errno(result);
    }

    return NULL;
}

static long vfs_romfs_telldir(DIR *dirp) {
    vfs_dir_t *dir = (vfs_dir_t *)dirp;
    romfs_off_t offset;

    offset = romfs_telldir(&fs, dir->fs_dir);
    if (offset < 0) {
        errno = romfs_to_errno(offset);
        return -1;
    }

    return (long)offset;
}

static int vfs_romfs_closedir(DIR *pdir) {
    vfs_dir_t *dir = ((vfs_dir_t *)pdir);
    int result;

    if ((result = romfs_dir_close(&fs, dir->fs_dir)) < 0) {
        errno = romfs_to_errno(result);
        return -1;
    }

    vfs_free_dir(dir);

    return 0;
}

static int vfs_romfs_mkdir(const char *path, mode_t mode) {
    errno = EROFS;
    return -1;
}

static int vfs_romfs_fsync(int fd) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    errno = EROFS;
    return -1;
}

static int vfs_romfs_ftruncate(int fd, off_t length) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    errno = EROFS;
    return -1;
}

static int vfs_romfs_truncate(const char *path, off_t length) {
    errno = EROFS;
    return -1;
}

int vfs_romfs_mount(const char *target) {
    esp_vfs_t vfs = {
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_romfs_write,
        .open = &vfs_romfs_open,
        .fstat = &vfs_romfs_fstat,
        .close = &vfs_romfs_close,
        .read = &vfs_romfs_read,
        .lseek = &vfs_romfs_lseek,
        .stat = &vfs_romfs_stat,
        .link = NULL,
        .unlink = &vfs_romfs_unlink,
        .rename = &vfs_romfs_rename,
        .mkdir = &vfs_romfs_mkdir,
        .opendir = &vfs_romfs_opendir,
        .readdir = &vfs_romfs_readdir,
        .closedir = &vfs_romfs_closedir,
        .rmdir = &vfs_romfs_rmdir,
        .fsync = &vfs_romfs_fsync,
        .access = &vfs_romfs_access,
        .telldir = &vfs_romfs_telldir,
        .truncate = &vfs_romfs_truncate,
        .ftruncate = &vfs_romfs_ftruncate,
    };

    // Mount the file system
    int res;
    romfs_config_t config;

    config.base = _romfs_start;

    if ((res = romfs_mount(&fs, &config)) == ROMFS_ERR_OK) {
        lstinit(&files, 0, LIST_DEFAULT);
        syslog(LOG_INFO, "romfs mounted on %s", target);

        // Register the file system
        ESP_ERROR_CHECK(esp_vfs_register("/romfs", &vfs, NULL));

        return 0;
    } else {
        syslog(LOG_ERR, "romfs mount error");
    }

    return -1;
}

int vfs_romfs_umount(const char *target) {
	romfs_umount(&fs);
	esp_vfs_unregister("/romfs");

    syslog(LOG_INFO, "romfs unmounted");

    return 0;
}

int vfs_romfs_fsstat(const char *target, u32_t *total, u32_t *used) {

    if (total) {
      *total = 0;
    }

    if (used) {
      *used = 0;
    }

    return 0;
}

#endif

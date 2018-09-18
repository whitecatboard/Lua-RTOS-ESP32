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
 * Lua RTOS lfs vfs
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_LFS

#include "rom/spi_flash.h"
#include "esp_partition.h"

#include <freertos/FreeRTOS.h>

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <dirent.h>

#include <sys/stat.h>

#include "esp_vfs.h"
#include <errno.h>

#include "lfs.h"

#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/mutex.h>
#include <sys/list.h>
#include <sys/fcntl.h>
#include <sys/vfs/vfs.h>
#include <dirent.h>

static int vfs_lfs_open(const char *path, int flags, int mode);
static ssize_t vfs_lfs_write(int fd, const void *data, size_t size);
static ssize_t vfs_lfs_read(int fd, void * dst, size_t size);
static int vfs_lfs_fstat(int fd, struct stat * st);
static int vfs_lfs_close(int fd);
static off_t vfs_lfs_lseek(int fd, off_t size, int mode);
static int vfs_lfs_access(const char *path, int amode);
static long vfs_lfs_telldir(DIR *dirp);

static struct list files;
static lfs_t lfs;

struct vfs_lfs_context {
    uint32_t base_addr;
    struct mtx lock;
};

/*
 * This function translate error codes from lfs to errno error codes
 *
 */
static int lfs_to_errno(int res) {
    switch (res) {
    case LFS_ERR_OK:
        return 0;

    case LFS_ERR_IO:
    case LFS_ERR_CORRUPT:
        return EIO;

    case LFS_ERR_NOENT:
        return ENOENT;

    case LFS_ERR_EXIST:
        return EEXIST;

    case LFS_ERR_NOTDIR:
        return ENOTDIR;

    case LFS_ERR_ISDIR:
        return EISDIR;

    case LFS_ERR_NOTEMPTY:
        return ENOTEMPTY;

    case LFS_ERR_BADF:
        return EBADF;

    case LFS_ERR_NOMEM:
        return ENOMEM;

    case LFS_ERR_NOSPC:
        return ENOSPC;

    case LFS_ERR_INVAL:
        return EINVAL;

    default:
        return res;
    }
}

static int vfs_lfs_open(const char *path, int flags, int mode) {
    int fd;
    int result;

    // Allocate new file
    vfs_file_t *file = calloc(1, sizeof(vfs_file_t));
    if (!file) {
        errno = ENOMEM;
        return -1;
    }

    file->fs_file = (void *)calloc(1, sizeof(lfs_file_t));
    if (!file->fs_file) {
        free(file);
        errno = ENOMEM;
        return -1;
    }

    file->path = strdup(path);
    if (!file->path) {
        free(file->fs_file);
        free(file);
        errno = ENOMEM;
        return -1;
    }

    // Add file to file list and get the file descriptor
    int res = lstadd(&files, file, &fd);
    if (res) {
        free(file->fs_file);
        free(file->path);
        free(file);
        errno = res;
        return -1;
    }

    // Translate flags to lfs flags
    int lfs_flags = 0;

    if (flags == O_APPEND)
        lfs_flags |= LFS_O_APPEND;

    if (flags == O_RDONLY)
        lfs_flags |= LFS_O_RDONLY;

    if (flags & O_WRONLY)
        lfs_flags |= LFS_O_WRONLY;

    if (flags & O_RDWR)
        lfs_flags |= LFS_O_RDWR;

    if (flags & O_EXCL)
        lfs_flags |= LFS_O_EXCL;

    if (flags & O_CREAT)
        lfs_flags |= LFS_O_CREAT;

    if (flags & O_TRUNC)
        lfs_flags |= LFS_O_TRUNC;

    if (*path == '/') {
        path++;
    }

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    if ((result = lfs_file_open(&lfs, file->fs_file, path, lfs_flags)) < 0) {
        errno = lfs_to_errno(result);
        lstremove(&files, fd, 0);

        free(file->fs_file);
        free(file->path);
        free(file);

        mtx_unlock(&ctx->lock);

        return -1;
    }

    mtx_unlock(&ctx->lock);

    return fd;
}

static ssize_t vfs_lfs_write(int fd, const void *data, size_t size) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    // Write to file
    result = lfs_file_write(&lfs, file->fs_file, (void *)data, size);
    if (result < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    mtx_unlock(&ctx->lock);

    return result;
}

static ssize_t vfs_lfs_read(int fd, void *dst, size_t size) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    // Read from file
    result = lfs_file_read(&lfs, file->fs_file, dst, size);
    if (result < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    mtx_unlock(&ctx->lock);

    return result;
}

static int vfs_lfs_fstat(int fd, struct stat *st) {
    vfs_file_t *file;
    struct lfs_info info;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    // Init stats
    memset(st, 0, sizeof(struct stat));

    // Set block size for this file system
    st->st_blksize = lfs.cfg->block_size;

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    // Get the file stats
    result = lfs_stat(&lfs, file->path, &info);
    if (result < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    mtx_unlock(&ctx->lock);

    st->st_size = info.size;
    st->st_mode = ((info.type==LFS_TYPE_REG)?S_IFREG:S_IFDIR);

    return 0;
}

static int vfs_lfs_close(int fd) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    // Close file
    result = lfs_file_close(&lfs, file->fs_file);
    if (result < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    // Remove file from file list
    lstremove(&files, fd, 0);

    free(file->fs_file);
    free(file->path);
    free(file);

    mtx_unlock(&ctx->lock);

    return 0;
}

static off_t vfs_lfs_lseek(int fd, off_t size, int mode) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    int whence = LFS_SEEK_CUR;

    switch (mode) {
    case SEEK_SET:
        whence = LFS_SEEK_SET;
        break;
    case SEEK_CUR:
        whence = LFS_SEEK_CUR;
        break;
    case SEEK_END:
        whence = LFS_SEEK_END;
        break;
    default:
        errno = EINVAL;
        return -1;
    }

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    result = lfs_file_seek(&lfs, file->fs_file, size, whence);
    if (result < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    mtx_unlock(&ctx->lock);

    return result;
}

static int vfs_lfs_stat(const char *path, struct stat *st) {
    struct lfs_info info;
    int result;

    // Init stats
    memset(st, 0, sizeof(struct stat));

    // Set block size for this file system
    st->st_blksize = lfs.cfg->block_size;

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    // Get the file stats
    result = lfs_stat(&lfs, path, &info);
    if (result < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    mtx_unlock(&ctx->lock);

    st->st_size = info.size;
    st->st_mode = ((info.type==LFS_TYPE_REG)?S_IFREG:S_IFDIR);

    return 0;
}

static int vfs_lfs_access(const char *path, int amode) {
#if 0
    struct stat s;

    if (vfs_lfs_stat(path, &s) < 0) {
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

static int vfs_lfs_unlink(const char *path) {
    struct lfs_info info;
    int result;

    // Sanity checks

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    // Is not a directory
    result = lfs_stat(&lfs, path, &info);
    if (result < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    if (info.type == LFS_TYPE_DIR) {
        mtx_unlock(&ctx->lock);
        errno = EPERM;
        return -1;
    }

    // Unlink
    if ((result = lfs_remove(&lfs, path)) < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    mtx_unlock(&ctx->lock);

    return 0;
}

static int vfs_lfs_rename(const char *src, const char *dst) {
    int result;

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    if ((result = lfs_rename(&lfs, src, dst)) < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    mtx_unlock(&ctx->lock);

    return 0;
}

static DIR* vfs_lfs_opendir(const char *name) {
    int result;

    vfs_dir_t *dir;

    dir = vfs_allocate_dir("lfs", name);
    if (!dir) {
        return NULL;
    }

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    // Open directory
    if ((result = lfs_dir_open(&lfs, dir->fs_dir, name)) < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        vfs_free_dir(dir);

        return NULL;
    }

    mtx_unlock(&ctx->lock);

    return (DIR *)dir;
}

static int vfs_lfs_rmdir(const char *name) {
    struct lfs_info info;
    int result;

    // Sanity checks
    if (strcmp(name,"/") == 0) {
        errno = EBUSY;
        return -1;
    }

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    // Is a directory
    result = lfs_stat(&lfs, name, &info);
    if (result < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    if (info.type != LFS_TYPE_DIR) {
        mtx_unlock(&ctx->lock);
        errno = ENOTDIR;
        return -1;
    }

    // Unlink
    if ((result = lfs_remove(&lfs, name)) < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    mtx_unlock(&ctx->lock);

    return 0;
}

static struct dirent *vfs_lfs_readdir(DIR *pdir) {
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

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

again:
    // Read next directory entry
    if ((result = lfs_dir_read(&lfs, ((vfs_dir_t *)pdir)->fs_dir, (struct lfs_info *)dir->fs_info)) < 0) {
        errno = lfs_to_errno(result);
    } else if (result > 0) {
        if ((strcmp(((struct lfs_info *)dir->fs_info)->name,".") == 0) || (strcmp(((struct lfs_info *)dir->fs_info)->name,"..") == 0)) {
            goto again;
        }

        ent->d_type = (((struct lfs_info *)dir->fs_info)->type == LFS_TYPE_REG)?DT_REG:DT_DIR;
        ent->d_fsize = ((struct lfs_info *)dir->fs_info)->size;
        strlcpy(ent->d_name, ((struct lfs_info *)dir->fs_info)->name, MAXNAMLEN);

        mtx_unlock(&ctx->lock);

        return ent;
    }

    mtx_unlock(&ctx->lock);

    return NULL;
}

static long vfs_lfs_telldir(DIR *dirp) {
    vfs_dir_t *dir = (vfs_dir_t *)dirp;

    lfs_soff_t offset;

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    offset = lfs_dir_tell(&lfs, dir->fs_dir);
    if (offset < 0) {
        mtx_unlock(&ctx->lock);
        errno = EBADF;
        return -1;
    }

    mtx_unlock(&ctx->lock);

    return (long)offset;
}

static int vfs_piffs_closedir(DIR *pdir) {
    vfs_dir_t *dir = ((vfs_dir_t *)pdir);
    int result;

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    if ((result = lfs_dir_close(&lfs, dir->fs_dir)) < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    vfs_free_dir(dir);

    mtx_unlock(&ctx->lock);

    return 0;
}

static int vfs_lfs_mkdir(const char *path, mode_t mode) {
    int result;

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    if ((result = lfs_mkdir(&lfs, path)) < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    mtx_unlock(&ctx->lock);

    return 0;
}

static int vfs_lfs_fsync(int fd) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    result = lfs_file_sync(&lfs, file->fs_file);
    if (result < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    mtx_unlock(&ctx->lock);

    return 0;
}

static int vfs_lfs_ftruncate(int fd, off_t length) {
    vfs_file_t *file;
    int result;

    // Get file from file list
    result = lstget(&files, fd, (void **) &file);
    if (result) {
        errno = EBADF;
        return -1;
    }

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)lfs.cfg->context;

    mtx_lock(&ctx->lock);

    result = lfs_file_truncate(&lfs, file->fs_file, length);
    if (result < 0) {
        mtx_unlock(&ctx->lock);
        errno = lfs_to_errno(result);
        return -1;
    }

    mtx_unlock(&ctx->lock);

    return 0;
}

static int lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)c->context;

    if (spi_flash_read(ctx->base_addr + (block * c->block_size) + off, buffer, size) != 0) {
        return LFS_ERR_IO;
    }

    return 0;
}

static int lfs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)c->context;

    if (spi_flash_write(ctx->base_addr + (block * c->block_size) + off, buffer, size) != 0) {
        return LFS_ERR_IO;
    }

    return 0;
}

static int lfs_erase(const struct lfs_config *c, lfs_block_t block) {
    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)c->context;

    if (spi_flash_erase_sector((ctx->base_addr + (block * c->block_size)) >> 12) != 0) {
        return LFS_ERR_IO;
    }

    return 0;
}

static int lfs_sync(const struct lfs_config *c) {
    return 0;
}

static struct lfs_config *lfs_config() {
    // Find a partition
    uint32_t base_address = 0;
    uint32_t fs_size = 0;

    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0xfe, "filesys");
    if (!partition) {
        syslog(LOG_ERR, "lfs can't find a valid partition");
        return NULL;
    } else {
        base_address = partition->address;
        fs_size = partition->size;
    }

    // Allocate file system configuration data
    struct lfs_config *cfg = calloc(1, sizeof(struct lfs_config));
    if (!cfg) {
        syslog(LOG_ERR, "lfs not enough memory");
        return NULL;
    }

    struct vfs_lfs_context *ctx = calloc(1, sizeof(struct vfs_lfs_context));
    if (!ctx) {
        free(cfg);

        syslog(LOG_ERR, "lfs not enough memory");
        return NULL;
    }

    // Configure the file system
    cfg->read  = lfs_read;
    cfg->prog  = lfs_prog;
    cfg->erase = lfs_erase;
    cfg->sync  = lfs_sync;

    cfg->block_size  = CONFIG_LUA_RTOS_LFS_BLOCK_SIZE;
    cfg->read_size   = CONFIG_LUA_RTOS_LFS_READ_SIZE;
    cfg->prog_size   = CONFIG_LUA_RTOS_LFS_PROG_SIZE;
    cfg->block_count = fs_size / cfg->block_size;
    cfg->lookahead   = cfg->block_count;

    cfg->context = ctx;
    ctx->base_addr = base_address;

    return cfg;
}

void vfs_lfs_mount() {
    esp_vfs_t vfs = {
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_lfs_write,
        .open = &vfs_lfs_open,
        .fstat = &vfs_lfs_fstat,
        .close = &vfs_lfs_close,
        .read = &vfs_lfs_read,
        .lseek = &vfs_lfs_lseek,
        .stat = &vfs_lfs_stat,
        .link = NULL,
        .unlink = &vfs_lfs_unlink,
        .rename = &vfs_lfs_rename,
        .mkdir = &vfs_lfs_mkdir,
        .opendir = &vfs_lfs_opendir,
        .readdir = &vfs_lfs_readdir,
        .closedir = &vfs_piffs_closedir,
        .rmdir = &vfs_lfs_rmdir,
        .fsync = &vfs_lfs_fsync,
        .access = &vfs_lfs_access,
        .ftruncate = &vfs_lfs_ftruncate,
        .telldir = &vfs_lfs_telldir,
    };

    // Register the file system
    ESP_ERROR_CHECK(esp_vfs_register("/lfs", &vfs, NULL));

    // Get configuration
    struct lfs_config *cfg = lfs_config();
    if (!cfg) {
        return;
    }

    struct vfs_lfs_context *ctx = (struct vfs_lfs_context *)(cfg->context);

    mtx_init(&ctx->lock, NULL, NULL, 0);

    syslog(LOG_INFO,
            "lfs start address at 0x%x, size %d Kb",
            ctx->base_addr, (cfg->block_count * cfg->block_size) / 1024);

    syslog(LOG_INFO, "lfs %d blocks, %d bytes/block, %d bytes/read, %d bytes/write",cfg->block_count,cfg->block_size, cfg->read_size, cfg->prog_size);

    // Mount file system
    int err = lfs_mount(&lfs, cfg);
    if (err < 0) {
        syslog(LOG_INFO, "lfs formatting ...");

        int block;
        for(block = 0;block < cfg->block_count;block++) {
          lfs_erase(cfg, block);
        }

        lfs_format(&lfs, cfg);
        err = lfs_mount(&lfs, cfg);
    }

    mount_set_mounted("lfs", (err == LFS_ERR_OK));

    if (err == LFS_ERR_OK) {
        lstinit(&files, 0, LIST_DEFAULT);

        syslog(LOG_INFO, "lfs mounted on %s", mount_get_mount_point_for_fs("lfs")->fpath);
    } else {
        free(cfg);
        free(ctx);

        syslog(LOG_INFO, "lfs mount error");
    }
}

void vfs_lfs_umount() {
    // Unmount
    lfs_umount(&lfs);

    // Free resources
    if (lfs.cfg) {
        if (lfs.cfg->context) {
            mtx_destroy(&((struct vfs_lfs_context *)lfs.cfg->context)->lock);
            free(lfs.cfg->context);
        }

        free((struct lfs_config *)lfs.cfg);
    }

    lstdestroy(&files, 1);

    // Unregister vfs
    esp_vfs_unregister("/lfs");

    // Mark as nor mounted
    mount_set_mounted("lfs", 0);

    syslog(LOG_INFO, "lfs unmounted");
}

void vfs_lfs_format() {
    // Unmount first
    vfs_lfs_umount();

    // Get configuration
    struct lfs_config *cfg = lfs_config();
    if (!cfg) {
        return;
    }

    // Format
    int block;
    for(block = 0;block < cfg->block_count;block++) {
      lfs_erase(cfg, block);
    }

    int err = lfs_format(&lfs, cfg);

    // Free resources
    if (lfs.cfg) {
        if (lfs.cfg->context) {
            free(lfs.cfg->context);
        }

        free((struct lfs_config *)lfs.cfg);
    }

    if (err == LFS_ERR_OK) {
        // Mount again
        vfs_lfs_mount();
    }
}

#endif

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
 * Lua RTOS common vfs functions
 *
 */

#include "esp_vfs.h"

#include <stdarg.h>
#include <unistd.h>

#include <sys/mount.h>

#define LUA_RTOS_SPIFFS_PART 0x40
#define LUA_RTOS_LFS_PART 0x41

typedef struct {
    void *fs_file;
    char *path;
    uint8_t is_dir;
} vfs_file_t;

typedef struct {
    DIR dir;
    long offset;
    void *fs_dir;
    void *fs_info;
    char *path;
    struct dirent ent;
    const struct mount_pt *mount;
} vfs_dir_t;

typedef struct {
	int flags; // FD flags
} vfs_fd_local_storage_t;

// Return if there are available bytes for read from the file descriptor.
// This function is blocking.
typedef int(*vfs_has_bytes)(int, int);

// Return the bytes available for write to the file descriptor.
// This function is non blocking.
typedef int(*vfs_free_bytes)(int);

// Get one byte from the file descriptor.
// This functions is blocking.
typedef int(*vfs_get_byte)(int,char *);

// Put one byte to the file descriptor.
// This function is blocking.
typedef void(*vfs_put_byte)(int,char *);

int vfs_fat_mount(const char *target);
int vfs_fat_umount(const char *target);
int vfs_fat_format(const char *target);

int vfs_spiffs_mount(const char *target);
int vfs_spiffs_umount(const char *target);
int vfs_spiffs_format(const char *target);

int vfs_lfs_mount(const char *target);
int vfs_lfs_umount(const char *target);
int vfs_lfs_format(const char *target);

int vfs_ramfs_mount(const char *target);
int vfs_ramfs_umount(const char *target);
int vfs_ramfs_format(const char *target);

int vfs_romfs_mount(const char *target);
int vfs_romfs_umount(const char *target);

int vfs_generic_fcntl(vfs_fd_local_storage_t *local_storage, int fd, int cmd, va_list args);
ssize_t vfs_generic_read(vfs_fd_local_storage_t *local_storage, vfs_has_bytes has_bytes, vfs_get_byte get, int fd, void * dst, size_t size);
ssize_t vfs_generic_write(vfs_fd_local_storage_t *local_storage, vfs_put_byte put, int fd, const void *data, size_t size);
ssize_t vfs_generic_writev(vfs_fd_local_storage_t *local_storage, vfs_put_byte put, int fd, const struct iovec *iov, int iovcnt);
int vfs_generic_select(vfs_fd_local_storage_t *local_storage, vfs_has_bytes has_bytes, vfs_free_bytes free, int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);

vfs_dir_t *vfs_allocate_dir(const char *vfs, const char *name);
void vfs_free_dir(vfs_dir_t *dir);
vfs_fd_local_storage_t *vfs_create_fd_local_storage(int num);

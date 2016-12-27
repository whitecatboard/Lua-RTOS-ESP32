/*
 * Lua RTOS, NET vfs operations
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 *
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "luartos.h"

#if USE_NET

#include "freertos/FreeRTOS.h"
#include "esp_vfs.h"
#include "esp_attr.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <fat/ff.h>

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syslog.h>

#include <syscalls/syscalls.h>

static int IRAM_ATTR vfs_net_open(const char *path, int flags, int mode);
static size_t IRAM_ATTR vfs_net_write(int fd, const void *data, size_t size);
static ssize_t IRAM_ATTR vfs_net_read(int fd, void * dst, size_t size);
static int IRAM_ATTR vfs_net_fstat(int fd, struct stat * st);
static int IRAM_ATTR vfs_net_close(int fd);
static off_t IRAM_ATTR vfs_net_lseek(int fd, off_t size, int mode);

static int IRAM_ATTR vfs_net_open(const char *path, int flags, int mode) {
    return 0;
}

static size_t IRAM_ATTR vfs_net_write(int fd, const void *data, size_t size) {
    return 0;
}

static ssize_t IRAM_ATTR vfs_net_read(int fd, void * dst, size_t size) {
	return 0;
}

static int IRAM_ATTR vfs_net_fstat(int fd, struct stat * st) {
    return 0;
}

static int IRAM_ATTR vfs_net_close(int fd) {
	return 0;
}

static off_t IRAM_ATTR vfs_net_lseek(int fd, off_t size, int mode) {
    return 0;
}

static int IRAM_ATTR vfs_net_stat(const char * path, struct stat * st) {
    return 0;
}

static int IRAM_ATTR vfs_net_unlink(const char *path) {
    return 0;
}

static int IRAM_ATTR vfs_net_rename(const char *src, const char *dst) {
    return 0;
}

void vfs_net_register() {
    esp_vfs_t vfs = {
        .fd_offset = 0,
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_net_write,
        .open = &vfs_net_open,
        .fstat = &vfs_net_fstat,
        .close = &vfs_net_close,
        .read = &vfs_net_read,
        .lseek = &vfs_net_lseek,
        .stat = &vfs_net_stat,
        .link = NULL,
        .unlink = &vfs_net_unlink,
        .rename = &vfs_net_rename
    };

    ESP_ERROR_CHECK(esp_vfs_register("/socket", &vfs, NULL));
}

#endif

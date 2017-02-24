/*
 * Lua RTOS, NET vfs operations
 *
 * Copyright (C) 2015 - 2017
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

#if USE_NET_VFS

#include "freertos/FreeRTOS.h"
#include "lwip/sockets.h"

#include "esp_vfs.h"
#include "esp_attr.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>

static int IRAM_ATTR vfs_net_open(const char *path, int flags, int mode);
static size_t IRAM_ATTR vfs_net_write(int fd, const void *data, size_t size);
static ssize_t IRAM_ATTR vfs_net_read(int fd, void * dst, size_t size);
static int IRAM_ATTR vfs_net_close(int fd);

static int IRAM_ATTR vfs_net_open(const char *path, int flags, int mode) {
	int s;

	// Get socket number
	sscanf(path,"/%d", &s);

    return s;
}

static size_t IRAM_ATTR vfs_net_write(int fd, const void *data, size_t size) {
	return (ssize_t)lwip_send(fd, data, size, 0);
}

static ssize_t IRAM_ATTR vfs_net_read(int fd, void * dst, size_t size) {
    return (ssize_t)lwip_recv(fd, dst, size, 0);
}

static int IRAM_ATTR vfs_net_close(int fd) {
	return closesocket(fd);
}

void vfs_net_register() {
    esp_vfs_t vfs = {
        .fd_offset = 0,
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_net_write,
        .open = &vfs_net_open,
        .fstat = NULL,
        .close = &vfs_net_close,
        .read = &vfs_net_read,
        .lseek = NULL,
        .stat = NULL,
        .link = NULL,
        .unlink = NULL,
        .rename = NULL
    };

    ESP_ERROR_CHECK(esp_vfs_register("/dev/socket", &vfs, NULL));
}

#endif

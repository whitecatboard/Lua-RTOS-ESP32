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

#if USE_NET_VFS

#include "freertos/FreeRTOS.h"
#include "lwip/sockets.h"

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

#include <syscalls/filedesc.h>
#include <syscalls/file.h>

extern struct file *get_file(int fd);
extern int closef(register struct file *fp);

static int IRAM_ATTR vfs_net_open(const char *path, int flags, int mode);
static size_t IRAM_ATTR vfs_net_write(int fd, const void *data, size_t size);
static ssize_t IRAM_ATTR vfs_net_read(int fd, void * dst, size_t size);
static int IRAM_ATTR vfs_net_close(int fd);

static int IRAM_ATTR vfs_net_open(const char *path, int flags, int mode) {
	struct file *fp;
	int fd, error;
	int s;

	// Allocate new file
    error = falloc(&fp, &fd);
    if (error) {
        errno = error;
        return -1;
    }

    // Get socket number
    sscanf(path,"/%d", &s);

    fp->f_fd      = fd;
    fp->f_fs      = NULL;
    fp->f_dir     = NULL;
    fp->f_path 	  = NULL;
    fp->f_fs_type = FS_SOCKET;
    fp->f_flag    = 0;
    fp->unit      = s;

    return fd;
}

static size_t IRAM_ATTR vfs_net_write(int fd, const void *data, size_t size) {
	struct file *fp;
	int bw;

	printf("vfs_net_write %d (%d)\r\n",fd,size);
	// Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

	bw = lwip_send(fp->unit, data, size, 0);
	return (ssize_t)bw;
}

static ssize_t IRAM_ATTR vfs_net_read(int fd, void * dst, size_t size) {
	struct file *fp;
	int br;

	printf("vfs_net_read %d (%d)\r\n",fd,size);

	// Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

    br = lwip_recv(fp->unit, dst, size, 0);
	return (ssize_t)br;
}

static int IRAM_ATTR vfs_net_close(int fd) {
	struct file *fp;

	// Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

	closesocket(fp->unit);
	closef(fp);

	return 0;
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

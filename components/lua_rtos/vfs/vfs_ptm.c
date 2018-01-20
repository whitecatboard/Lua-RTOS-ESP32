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
 * Lua RTOS PTM VFS (master pseudo terminal)
 * Lua RTOS PTS VFS (slave pseudo terminal)
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_SSH_SERVER

#include "vfs.h"

#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/errno.h>
#include <sys/lock.h>
#include <sys/fcntl.h>
#include "esp_attr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Number of opened masters and slaves
// Only 1 master / slave allowed
static uint8_t slaves = 0;
static uint8_t masters = 0;

// Local storage for file descriptors
static vfs_fd_local_storage_t *slave_local_storage;
static vfs_fd_local_storage_t *master_local_storage;

static xQueueHandle slave_q = NULL;
static xQueueHandle master_q = NULL;

// Register master functions
static int vfs_ptm_open(const char *path, int flags, int mode);
static ssize_t vfs_ptm_write(int fd, const void *data, size_t size);
static ssize_t vfs_ptm_read(int fd, void * dst, size_t size);
static int vfs_ptm_close(int fd);
static ssize_t vfs_ptm_writev(int fd, const struct iovec *iov, int iovcnt);
static int vfs_ptm_select (int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
static int vfs_ptm_fcntl(int fd, int cmd, va_list args);

static void register_master() {
    esp_vfs_t vfs = {
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_ptm_write,
        .open = &vfs_ptm_open,
        .fstat = NULL,
        .close = &vfs_ptm_close,
        .read = &vfs_ptm_read,
        .fcntl = &vfs_ptm_fcntl,
        .ioctl = NULL,
        .writev = &vfs_ptm_writev,
		.select = &vfs_ptm_select,
    };

    ESP_ERROR_CHECK(esp_vfs_register("/dev/ptm", &vfs, NULL));
}

// Register slave functions
static int vfs_pts_open(const char *path, int flags, int mode);
static ssize_t vfs_pts_write(int fd, const void *data, size_t size);
static ssize_t vfs_pts_read(int fd, void * dst, size_t size);
static int vfs_pts_close(int fd);
static ssize_t vfs_pts_writev(int fd, const struct iovec *iov, int iovcnt);
static int vfs_pts_select (int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
static int vfs_pts_fcntl(int fd, int cmd, va_list args);

static void register_slave() {
    esp_vfs_t vfs = {
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_pts_write,
        .open = &vfs_pts_open,
        .fstat = NULL,
        .close = &vfs_pts_close,
        .read = &vfs_pts_read,
        .fcntl = &vfs_pts_fcntl,
        .ioctl = NULL,
        .writev = &vfs_pts_writev,
		.select = &vfs_pts_select,
    };

    ESP_ERROR_CHECK(esp_vfs_register("/dev/pts", &vfs, NULL));
}

// Register vfs
void vfs_pty_register() {
	register_master();
	register_slave();

	slave_local_storage = vfs_create_fd_local_storage(1);
	assert(slave_local_storage != NULL);

	master_local_storage = vfs_create_fd_local_storage(1);
	assert(master_local_storage != NULL);

	// Create the slave and master queues
	slave_q = xQueueCreate(1024, sizeof(char));
	assert(slave_q != NULL);

	master_q = xQueueCreate(1024, sizeof(char));
	assert(master_q != NULL);
}

// Master functions
static int master_has_bytes(int fd, int to) {
	char c;

	if (to != portMAX_DELAY) {
		to = to / portTICK_PERIOD_MS;
	}

	return (xQueuePeek(master_q, &c, (BaseType_t)to) == pdTRUE);
}

static int master_free(int fd) {
	return uxQueueSpacesAvailable(master_q);
}

static int master_get(int fd, char *c) {
	return xQueueReceive(master_q, c, portMAX_DELAY);
}

static void master_put(int fd, char *c) {
	xQueueSend(slave_q, c, portMAX_DELAY);
}

static int vfs_ptm_open(const char *path, int flags, int mode) {
	if (masters > 0) {
		errno = ENOENT;
		return -1;
	}

	masters++;
	master_local_storage[0].flags = flags;

	return 0;
}

static int vfs_ptm_close(int fd) {
	if (masters > 0) {
		masters--;
	}

	if (master_q) {
		xQueueReset(master_q);
	}

	return 0;
}

static ssize_t vfs_ptm_read(int fd, void * dst, size_t size) {
	return vfs_generic_read(master_local_storage, master_has_bytes, master_get, fd, dst, size);
}

static ssize_t vfs_ptm_write(int fd, const void *data, size_t size) {
	return vfs_generic_write(master_local_storage, master_put, fd, data, size);
}

static ssize_t vfs_ptm_writev(int fd, const struct iovec *iov, int iovcnt) {
	return vfs_generic_writev(master_local_storage, master_put, fd, iov, iovcnt);
}

static int vfs_ptm_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout) {
	return vfs_generic_select(master_local_storage, master_has_bytes, master_free, maxfdp1, readset, writeset, exceptset, timeout);
}

static int vfs_ptm_fcntl(int fd, int cmd, va_list args) {
	return vfs_generic_fcntl(master_local_storage, fd, cmd, args);
}

// Slave functions
static int slave_has_bytes(int fd, int to) {
	char c;

	if (to != portMAX_DELAY) {
		to = to / portTICK_PERIOD_MS;
	}

	return (xQueuePeek(slave_q, &c, (BaseType_t)to) == pdTRUE);
}

static int slave_free(int fd) {
	return uxQueueSpacesAvailable(slave_q);
}

static int slave_get(int fd, char *c) {
	return xQueueReceive(slave_q, c, portMAX_DELAY);
}

static void slave_put(int fd, char *c) {
	xQueueSend(master_q, c, portMAX_DELAY);
}

static int vfs_pts_open(const char *path, int flags, int mode) {
	if (slaves > 0) {
		errno = ENOENT;
		return -1;
	}

	slaves++;
	slave_local_storage[0].flags = flags;

	return 0;

}

static int vfs_pts_close(int fd) {
	if (slaves > 0) {
		slaves--;
	}

	if (slave_q) {
		xQueueReset(slave_q);
	}

	return 0;
}

static ssize_t vfs_pts_write(int fd, const void *data, size_t size) {
	return vfs_generic_write(slave_local_storage, slave_put, fd, data, size);
}

static ssize_t vfs_pts_read(int fd, void * dst, size_t size) {
	return vfs_generic_read(slave_local_storage, slave_has_bytes, slave_get, fd, dst, size);
}

static ssize_t vfs_pts_writev(int fd, const struct iovec *iov, int iovcnt) {
	return vfs_generic_writev(slave_local_storage, slave_put, fd, iov, iovcnt);
}

static int vfs_pts_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout) {
	return vfs_generic_select(slave_local_storage, slave_has_bytes, slave_free, maxfdp1, readset, writeset, exceptset, timeout);
}

static int vfs_pts_fcntl(int fd, int cmd, va_list args) {
	return vfs_generic_fcntl(slave_local_storage, fd, cmd, args);
}

#endif

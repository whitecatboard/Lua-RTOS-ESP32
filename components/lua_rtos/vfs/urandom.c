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
 * Lua RTOS urandom vfs
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_SSH_SERVER

#include "vfs.h"

static int registered = 0;

extern int os_get_random(unsigned char *buf, size_t len);

static int vfs_urandom_open(const char *path, int flags, int mode) {
    return 0;
}

static ssize_t vfs_urandom_read(int fd, void * dst, size_t size) {
	os_get_random(dst, size);
	
	return size;
}

static int vfs_urandom_close(int fd) {
	return 0;
}

static int vfs_urandom_select (int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout) {
	int num = 0;
	int fd;

	for(fd = 0;fd <= maxfdp1;fd++) {
		if (readset && FD_ISSET(fd, readset)) {
			num++;
		}

		if (writeset && FD_ISSET(fd, writeset)) {
			num++;
		}

		if (exceptset && FD_ISSET(fd, exceptset)) {
		}
	}

	return num;
}

void vfs_urandom_register() {
	if (!registered) {
	    esp_vfs_t vfs = {
	    	.flags = ESP_VFS_FLAG_DEFAULT,
	        .write = NULL,
	        .open = &vfs_urandom_open,
	        .fstat = NULL,
	        .close = &vfs_urandom_close,
	        .read = &vfs_urandom_read,
	        .lseek = NULL,
	        .stat = NULL,
	        .link = NULL,
	        .unlink = NULL,
	        .rename = NULL,
			.select = &vfs_urandom_select,
	    };

	    ESP_ERROR_CHECK(esp_vfs_register("/dev/urandom", &vfs, NULL));

	    registered = 1;
	}
}

#endif

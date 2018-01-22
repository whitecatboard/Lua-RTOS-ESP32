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

#include "vfs.h"

#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/fcntl.h>

#define MAX(a,b) (((a)>(b))?(a):(b))

vfs_fd_local_storage_t *vfs_create_fd_local_storage(int num) {
	vfs_fd_local_storage_t *ptr;

	ptr = calloc(num, sizeof(vfs_fd_local_storage_t));
	if (!ptr) {
		errno = ENOMEM;
		return NULL;
	}

	return ptr;
}

void vfs_destroy_fd_local_storage(vfs_fd_local_storage_t *ptr) {
	free(ptr);
}

int vfs_generic_fcntl(vfs_fd_local_storage_t *local_storage, int fd, int cmd, va_list args) {
    int result = 0;

    if (cmd == F_GETFL) {
    	if (local_storage) {
    		return local_storage[fd].flags;
    	} else {
    		return 0;
    	}
    } else if (cmd == F_SETFL) {
    	if (local_storage) {
    		local_storage[fd].flags = va_arg(args, int);
    	}
    } else {
        result = -1;
        errno = ENOSYS;
    }

    return result;
}

ssize_t vfs_generic_read(vfs_fd_local_storage_t *local_storage, vfs_has_bytes has_bytes, vfs_get_byte get, int fd, void * dst, size_t size) {
	char *c = (char *)dst;
	int bytes = 0;

	while (size) {
		if (local_storage && (local_storage[fd].flags & O_NONBLOCK)) {
			if (!has_bytes(fd, 0)) {
				if (bytes > 0) {
					return bytes;
				}

				errno = EAGAIN;
				return -1;
			}
		} else {
			if (!has_bytes(fd, 0) && (bytes > 0)) {
				return bytes;
			}
		}

		if (!get(fd, c)) {
			errno = EIO;
			return -1;
		}

		c++;
		size--;
		bytes++;
	}

	return bytes;
}

ssize_t vfs_generic_write(vfs_fd_local_storage_t *local_storage, vfs_put_byte put, int fd, const void *data, size_t size) {
	char *c = (char *)data;
	int bytes = 0;

	while (size) {
#if CONFIG_NEWLIB_STDOUT_LINE_ENDING_LF
		char n;

		if (*c=='\n') {
        	n = '\r';
        	put(fd, &n);
        }
#endif

        put(fd, c);
		c++;
		size--;
		bytes++;
	}

	return bytes;
}

ssize_t vfs_generic_writev(vfs_fd_local_storage_t *local_storage, vfs_put_byte put, int fd, const struct iovec *iov, int iovcnt) {
	int bytes = 0;
	int len = 0;
	char *c;

	while (iovcnt) {
		c = (char *)iov->iov_base;
		len = iov->iov_len;

		while (len) {
			put(fd, c);
			c++;
			len--;
			bytes++;
		}
		iov++;
		iovcnt--;
	}

	return bytes;
}

int vfs_generic_select(vfs_fd_local_storage_t *local_storage, vfs_has_bytes has_bytes, vfs_free_bytes free, int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout) {
	int to = 0xffffffff; // Default timeout

	// Get the timeout
	if (timeout) {
		to = MAX((timeout->tv_sec * 1000 + (timeout->tv_usec + 500) / 1000),1);
	}

	// Inspect all the file descriptors
	int num = 0; // Number of available file descriptors
	int fd;      // Current inspected file descriptor

	for(fd = 0;fd <= maxfdp1;fd++) {
		if (readset && FD_ISSET(fd, readset)) {
			if (has_bytes(fd, to)) {
				num++;
			} else {
				FD_CLR(fd, readset);
			}
		}

		if (writeset && FD_ISSET(fd, writeset)) {
			if (free(fd) > 0) {
				num++;
			} else {
				FD_CLR(fd, writeset);
			}
		}

		if (exceptset && FD_ISSET(fd, exceptset)) {
		}
	}

	return num;
}

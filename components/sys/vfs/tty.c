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
 * Lua RTOS tty vfs
 *
 */

#include "sdkconfig.h"
#include "vfs.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>

#include <sys/stat.h>

#include <drivers/uart.h>

extern FILE *lua_stdout_file;

// Local storage for file descriptors
static vfs_fd_local_storage_t *local_storage;

static int has_bytes(int fd, int to) {
	char c;

	if (to != portMAX_DELAY) {
		to = to / portTICK_PERIOD_MS;
	}

	return (xQueuePeek(uart_get_queue(fd), &c, (BaseType_t)to) == pdTRUE);
}

static int get(int fd, char *c) {
	return uart_read(fd, c, portMAX_DELAY);
}

static void put(int fd, char *c) {
    uart_write(fd, *c);
    if (lua_stdout_file) {
    	fwrite(c, 1, 1, lua_stdout_file);
    }
}

static int  vfs_tty_open(const char *path, int flags, int mode) {
	int unit = 0;

	// Get UART unit
    if (strcmp(path, "/0") == 0) {
    	unit = 0;
    } else if (strcmp(path, "/1") == 0) {
    	unit = 1;
    } else if (strcmp(path, "/2") == 0) {
    	unit = 2;
    } else {
		errno = ENOENT;
    	return -1;
	}

    // Init uart unit
    uart_init(unit, CONSOLE_BR, 8, 0, 1, UART_FLAG_READ | UART_FLAG_WRITE, CONSOLE_BUFFER_LEN);
    uart_setup_interrupts(unit);

    // Store flags
    local_storage[unit].flags = flags;

    return unit;
}

static ssize_t vfs_tty_write(int fd, const void *data, size_t size) {
	int ret;

    uart_ll_lock(fd);
	ret = vfs_generic_write(local_storage, put, fd, data, size);
    uart_ll_unlock(fd);

    return ret;
}

static ssize_t vfs_tty_read(int fd, void * dst, size_t size) {
	return vfs_generic_read(local_storage, has_bytes, get, fd, dst, size);
}

static int vfs_tty_fstat(int fd, struct stat * st) {
    st->st_mode = S_IFCHR;
    return 0;
}

static int vfs_tty_close(int fd) {
	return 0;
}

static ssize_t vfs_tty_writev(int fd, const struct iovec *iov, int iovcnt) {
	int ret;

    uart_ll_lock(fd);
	ret = vfs_generic_writev(local_storage, put, fd, iov, iovcnt);
    uart_ll_unlock(fd);

    return ret;
}

static int vfs_tty_fcntl(int fd, int cmd, va_list args) {
	return vfs_generic_fcntl(local_storage, fd, cmd, args);
}

void vfs_tty_register() {
    esp_vfs_t vfs = {
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_tty_write,
        .open = &vfs_tty_open,
        .fstat = &vfs_tty_fstat,
        .close = &vfs_tty_close,
        .read = &vfs_tty_read,
        .lseek = NULL,
        .stat = NULL,
        .link = NULL,
        .unlink = NULL,
        .rename = NULL,
		.fcntl = &vfs_tty_fcntl,
		.writev = &vfs_tty_writev,
    };
	
    ESP_ERROR_CHECK(esp_vfs_register("/dev/tty", &vfs, NULL));

	local_storage = vfs_create_fd_local_storage(3);
	assert(local_storage != NULL);

    // Close previous standard streams
	if (_GLOBAL_REENT->_stdin)
		fclose(_GLOBAL_REENT->_stdin);

	if (_GLOBAL_REENT->_stdout)
		fclose(_GLOBAL_REENT->_stdout);

	if (_GLOBAL_REENT->_stderr)
		fclose(_GLOBAL_REENT->_stderr);
	
	// Open standard streams, using defined tty for console
	if (CONSOLE_UART == 0) {
		_GLOBAL_REENT->_stdin  = fopen("/dev/tty/0", "r");
		_GLOBAL_REENT->_stdout = fopen("/dev/tty/0", "w");
		_GLOBAL_REENT->_stderr = fopen("/dev/tty/0", "w");
	}

	if (CONSOLE_UART == 1) {
		_GLOBAL_REENT->_stdin  = fopen("/dev/tty/1", "r");
		_GLOBAL_REENT->_stdout = fopen("/dev/tty/1", "w");
		_GLOBAL_REENT->_stderr = fopen("/dev/tty/1", "w");
	}

	if (CONSOLE_UART == 2) {
		_GLOBAL_REENT->_stdin  = fopen("/dev/tty/2", "r");
		_GLOBAL_REENT->_stdout = fopen("/dev/tty/2", "w");
		_GLOBAL_REENT->_stderr = fopen("/dev/tty/2", "w");
	}

	// Work-around newlib is not compiled with HAVE_BLKSIZE flag
	setvbuf(_GLOBAL_REENT->_stdin , NULL, _IONBF, 0);
	setvbuf(_GLOBAL_REENT->_stdout, NULL, _IONBF, 0);
	setvbuf(_GLOBAL_REENT->_stderr, NULL, _IONBF, 0);
}

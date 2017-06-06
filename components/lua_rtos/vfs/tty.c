/*
 * Lua RTOS, tty vfs operations
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

#include "esp_vfs.h"
#include "esp_attr.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <sys/stat.h>

#include <pthread/pthread.h>

#include <drivers/uart.h>

extern FILE *lua_stdout_file;

static int IRAM_ATTR vfs_tty_open(const char *path, int flags, int mode) {
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

    return unit;
}

static size_t IRAM_ATTR vfs_tty_write(int fd, const void *data, size_t size) {
    const char *data_c = (const char *)data;
    int unit = fd;

    uart_ll_lock(unit);

    for (size_t i = 0; i < size; i++) {
#if CONFIG_NEWLIB_STDOUT_ADDCR
        if (data_c[i]=='\n') {
        	uart_write(unit, '\r');
        }
#endif
        uart_write(unit, data_c[i]);
        if (lua_stdout_file) {
        	fwrite(&data_c[i], 1, 1, lua_stdout_file);
        }
    }

	uart_ll_unlock(unit);

    return size;
}

static ssize_t IRAM_ATTR vfs_tty_read(int fd, void * dst, size_t size) {
	ssize_t remain = (ssize_t)size;
	int unit = fd;

	while (remain) {
        if (uart_read(unit, (char *)dst, portMAX_DELAY)) {
			remain--;
			dst++;
        } else {
            break;
        } 
    }	
	
	return size - remain;
}

static int IRAM_ATTR vfs_tty_fstat(int fd, struct stat * st) {
    st->st_mode = S_IFCHR;
    return 0;
}

static int IRAM_ATTR vfs_tty_close(int fd) {
	return 0;
}

void vfs_tty_register() {
    esp_vfs_t vfs = {
        .fd_offset = 0,
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
        .rename = NULL
    };
	
    ESP_ERROR_CHECK(esp_vfs_register("/dev/tty", &vfs, NULL));

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

/*
 * Lua RTOS, mkdir sys call implementation
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

#include "syscalls.h"

#include <spiffs.h>
#include <esp_spiffs.h>
#include <spiffs_nucleus.h>

#include <fat/ff.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/mount.h>

extern spiffs fs;
extern int fat_result(int res);
extern int spiffs_result(int res);

static int _mkdir_fat(const char *path, mode_t mode) {
	int res;

	res = f_mkdir(path);
	if (res != 0) {
		errno = fat_result(res);
		return -1;
	}

	return 0;
}

static int _mkdir_spiffs(const char *path, mode_t mode) {
    char npath[PATH_MAX + 1];
    int res;

    // Add /. to path
    strncpy(npath, path, PATH_MAX);
    if ((strcmp(path,"/") != 0) && (strcmp(path,"/.") != 0)) {
        strncat(npath,"/.", PATH_MAX);
    }

    spiffs_file fd = SPIFFS_open(&fs, npath, SPIFFS_CREAT, 0);
    if (fd < 0) {
        res = spiffs_result(fs.err_code);
        errno = res;
        return -1;
    }

    if (SPIFFS_close(&fs, fd) < 0) {
        res = spiffs_result(fs.err_code);
        errno = res;
        return -1;
    }

    return 0;
}

int mkdir(const char *path, mode_t mode) {
	const char *device;
	char *ppath;
	char *rpath;
	int fd;

	// Get physical path
	ppath = mount_resolve_to_physical(path);
	if (!ppath) {
		errno = EFAULT;
		return -1;
	}

	// Test for file existence
	errno = 0;

	if ((fd = open(ppath, 0))) {
		if (errno == 0) {
			// File exists
			free(ppath);
			close(fd);
			errno = EEXIST;
			return -1;
		}
	}

	// File doesn't exists, create directory
	close(fd);

	// Get device
	device = mount_get_device_from_path(ppath, &rpath);
	if (!device) {
		free(ppath);
		errno = EFAULT;
		return -1;
	}

	free(ppath);

	if (strcmp("fat",device) == 0) {
		return _mkdir_fat(rpath, mode);
	} else if (strcmp("spiffs",device) == 0) {
		return _mkdir_spiffs(rpath, mode);
	} else {
		errno = EFAULT;
		return -1;
	}

	return 0;
}


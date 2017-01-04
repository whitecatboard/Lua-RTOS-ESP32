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

#include "luartos.h"

#include "syscalls.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>

int vfs_spiffs_mkdir(const char *path, mode_t mode);
int vfs_fat_mkdir(const char *path, mode_t mode);

int mkdir(const char *path, mode_t mode) {
	const char *device;
	char *ppath;
	char *rpath;
	int res = 0;
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

#if USE_FAT
	if (strcmp("fat",device) == 0) {
		res = vfs_fat_mkdir(rpath, mode);
	}
#endif

#if USE_SPIFFS
	if (strcmp("spiffs",device) == 0) {
		res = vfs_spiffs_mkdir(rpath, mode);
	}
#endif

	free(ppath);

	return res;
}

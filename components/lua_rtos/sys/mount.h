/*
 * Lua RTOS, minimal mount capabilities. Mount are only allowed in
 * the default root's directory.
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

#ifndef _SYSCALLS_MOUNT_H
#define	_SYSCALLS_MOUNT_H

char *mount_normalize_path(const char *path);
char *mount_readdir(const char *dev, const char*path, int idx, char *buf);
const char *mount_device(const char *path);
const char *mount_path(const char *path);
void mount_set_mounted(const char *device, unsigned int mounted);
int mount_is_mounted(const char *device);
const char *mount_default_device();
char *mount_resolve_to_physical(const char *path);
char *mount_resolve_to_logical(const char *path);
const char *mount_get_device_from_path(const char *path, char **rpath);
const char *mount_get_mount_from_path(const char *path, char **rpath);
int mount_is_mounted(const char *device);

#endif


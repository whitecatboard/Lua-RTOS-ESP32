/*
 * Whitecat, minimal mount capabilities. Mount are only allowed in
 * the default root's directory.
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

#ifndef _SYSCALLS_MOUNT_H
#define	_SYSCALLS_MOUNT_H

int mount_dirs(const char *dev, const char*path);
char *mount_readdir(const char *dev, const char*path, int idx, char *buf);
const char *mount_device(const char *path);
char *mount_root(char *path);
void mount_set_mounted(const char *device, unsigned int mounted);
char *mount_primary_or_secondary(const char *path);
char *mount_secondary_or_primary(const char *path);
int mount_is_mounted(const char *device);
int primary_is_mounted();
const char *mount_default_device();
int primary_is_mounted();

#endif


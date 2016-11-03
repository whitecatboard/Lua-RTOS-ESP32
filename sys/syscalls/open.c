/*
 * Lua RTOS, open syscall implementation
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

#include <reent.h>
#include <string.h>

#include <sys/syscalls/mount.h>

extern struct filedesc *p_fd;
extern const struct device devs[];
extern const int ndevs;

// This function opens the device linked to a file name, and assign the
// device low-level operations needed for access the file
//
// File name can belong to the device tree, or the file system
//
// Device tree: /dev/DDDU, where DDD is the device name, and U the device unit
static int devopen(struct file *fp, char *fname) {
	char tmp[PATH_MAX + 5];
	char *dev_name;
	int unit = 0;
    char *c;
	
    c = fname;

    if (*c != '/') {
        // File name is relative, only can belong to the file system
        goto fs;
    }
    
	c++;

    // File name is absolute, we must test if it's part of the device tree
    // or the file system
	while ((*c != '/') && (*c != '\0')) {
        c++;
    }
    
    if (*c == '/') {
        if (strncmp(fname, "/dev/", (c - fname - 1)) == 0) {
            // File name is /dev/xxxx, only can belong to device tree
            goto device;
        } else {
            // File name is /xxxx, only can belong to the file system
            goto fs;
        }
    } else {
        // File name is /xxxx, only can belong to the file system
        goto fs;
    }
    
device:
	// File name belongs to the device tree
	dev_name = ++c;

	// Get unit
    while (((*c < '0') || (*c > '9')) && (*c != '\0')) {
		c++;
    }

    if ((*c >= '0') && (*c <= '9')) {
        unit = ((int)(*c)) - (int)'0';
    } else {
        return (ENXIO);
    }

    // Store device unit
    fp->f_devunit = unit;

	c--;
	
    goto find;
    
fs:    
	strcpy(tmp, mount_device(fname));

    if (!mount_is_mounted(tmp)) {
        return (ENXIO);   
    }

	dev_name = tmp;
	c = tmp + strlen(tmp) - 1;

find:	
    // Store device name
    fp->f_devname = (char *)calloc(1, sizeof(char) * (c - dev_name + 2));
    if (!fp->f_devname) {
        return ENOMEM;
    }
        
    memcpy(fp->f_devname, dev_name, c - dev_name + 1);

    // Find device and get it's ops
	int i;
	
    for(i=0;i < ndevs;i++) {
        if (strcmp(devs[i].d_name, fp->f_devname) == 0) {
            fp->f_ops = (struct fileops *)&(devs[i].d_ops);
            return 0;
        }
    }
    
    return (ENXIO);        
}

int __open(struct _reent *r, const char *path, int flags, ...) {
    register struct filedesc *fdp = p_fd;
    struct file *fp;
    int fd, error;

    // Alloc file
    error = falloc(&fp, &fd);
    if (error) {
        __errno_r(r) = error;
        return -1;
    }
    
    // Normalize path
    fp->f_path = normalize_path(path);
    if (!fp->f_path) {
        closef(fp);
        fdp->fd_ofiles[fd] = NULL;
        return -1;
    }

    fp->f_flag = FFLAGS(flags) & FMASK;

    // Open device, and assign to file
    error = devopen(fp, fp->f_path);
    if (error) {
        __errno_r(r) = error;
        closef(fp);
        fdp->fd_ofiles[fd] = NULL;
        return -1;
    }
    
    fp->f_path = mount_root(fp->f_path);

    // Try to open 
    error = (*fp->f_ops->fo_open)(fp, flags);
    if (error) {
        __errno_r(r) = error;
        closef(fp);
        fdp->fd_ofiles[fd] = NULL;
        return -1;
    }
    
    return fd;
}

#if ((PLATFORM_ESP32 != 1) && (PLATFORM_ESP8266 != 1))

#include <stdarg.h>
int open(const char *path, int flags, ...) {
	va_list ap;
	int ret;
	
	va_start(ap, flags);
	ret = __open(_GLOBAL_REENT, path, flags, ap);	
	va_end(ap);
	
	return ret;
}

#endif

/*
 * Whitecat, minimal open syscall implementation
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

#include <string.h>
#include <stdarg.h>

#include <sys/syscalls/mount.h>

extern struct filedesc *p_fd;
extern const struct device devs[];
extern const int ndevs;

char *normalize_path(const char *path) {
    char *rpath;
    char *cpath;
    char *tpath;
    char *last;
    int maybe_is_dot = 0;
    int maybe_is_dot_dot = 0;
    int is_dot = 0; 
    int is_dot_dot = 0;
    int plen = 0;
    
    rpath = malloc(MAXPATHLEN);
    if (!rpath) {
        errno = ENOMEM;
        return NULL;
    }
    
    // If it's a relative path preappend current working directory
    if (*path != '/') {
        if (!getcwd(rpath, MAXPATHLEN)) {
            free(rpath);
            return NULL;
        }
         
        if (*(rpath + strlen(rpath) - 1) != '/') {
            rpath = strcat(rpath, "/");  
        }
        
        rpath = strcat(rpath, path);        
    } else {
        strcpy(rpath, path);
    }
    
    plen = strlen(rpath);
    if (*(rpath + plen - 1) != '/') {
        rpath = strcat(rpath, "/");  
        plen++;
    }
    
    cpath = rpath;
    while (*cpath) {
        if (*cpath == '.') {
            if (maybe_is_dot) {
                maybe_is_dot_dot = 1;
                maybe_is_dot = 0;
            } else {
                maybe_is_dot = 1;
            }
        } else {
            if (*cpath == '/') {
                is_dot_dot = maybe_is_dot_dot;
                is_dot = maybe_is_dot && !is_dot_dot;
            } else {
                maybe_is_dot_dot = 0;
                maybe_is_dot = 0;
            }
        }

        if (is_dot_dot) {
            last = cpath + 1;
            
            while (*--cpath != '/');
            while (*--cpath != '/');
            
            tpath = ++cpath;
            while (*last) {
                *tpath++ = *last++;
            }
            *tpath = '\0';
            
            is_dot_dot = 0;
            is_dot = 0;
            maybe_is_dot = 0;
            maybe_is_dot_dot = 0; 
            continue;
        }        

        if (is_dot) {
            last = cpath + 1;
            
            while (*--cpath != '/');
            
            tpath = ++cpath;
            while (*last) {
                *tpath++ = *last++;
            }
            *tpath = '\0';
            
            is_dot_dot = 0;
            is_dot = 0;
            maybe_is_dot = 0;
            maybe_is_dot_dot = 0; 
            continue;
        }        
        
        cpath++;           
    }
    
    cpath--;
    if ((cpath != rpath) && (*cpath == '/')) {
        *cpath = '\0';
    }
    
    return rpath;
}

int devopen(fp, fname)
    struct file *fp;
    char *fname;
{
    register char *ncp;
    register char *c;
    register int i;
    int unit = 0;
    char namebuf[20];

    c = fname;
    ncp = namebuf;

    if (*c != '/') {
        // File name is relative, only can belong to the file system
        goto fs;
    }
    
    *ncp++ = *c++;
    
    // File name is absolute, we must test if it's part of the device tree
    // or the file system
    while ((*c != '/') && (*c != '\0')) {
        *ncp++ = *c++;
    }
    
    if (*c == '/') {
        *ncp++ = *c++;
        *ncp = '\0';
        
        if (strcmp(namebuf, "/dev/") == 0) {
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
    // Get the device name, and unit
    ncp = namebuf;

    while (((*c < '0') || (*c > '9')) && (*c != '\0')) {
        *ncp++ = *c++;
    }

    *ncp = '\0';

    if ((*c >= '0') && (*c <= '9')) {
        unit = ((int)(*c)) - (int)'0';
    } else {
        return (ENXIO);
    }
    
    // Store device unit
    fp->f_devunit = unit;

    // namebuff has the device name, example: tty
    // unit has the unit number of the device

    goto find;
    
fs:
    strcpy(namebuf, mount_device(fname));
    if (!mount_is_mounted(namebuf)) {
        return (ENXIO);   
    }

find:
    // store device name
    fp->f_devname = (char *)malloc(sizeof(char) * strlen(namebuf));
    if (!fp->f_devname) {
        return ENOMEM;
    }
        
    strcpy(fp->f_devname, namebuf);

    // find device and get it's ops
    for(i=0;i < ndevs;i++) {
        if (strcmp(devs[i].d_name, namebuf) == 0) {
            fp->f_ops = (struct fileops *)&(devs[i].d_ops);
            return 0;
        }
    }
    
  
    return (ENXIO);        
}
    
/*
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int open(const char *path, int flags, ...) {
    register struct filedesc *fdp = p_fd;
    struct file *fp;
    int indx, error;
    
    // Alloc file
    error = falloc(&fp, &indx);
    if (error) {
        errno = error;
        return -1;
    }
    
    // Normalize path and store into file
    fp->f_path = normalize_path(path);
    if (!fp->f_path) {
        closef(fp);
        fdp->fd_ofiles[indx] = NULL;
        return -1;
    }

    fp->f_flag = FFLAGS(flags) & FMASK;

    // Open device, and assign to file
    error = devopen(fp, fp->f_path);
    if (error) {
        errno = error;
        closef(fp);
        fdp->fd_ofiles[indx] = NULL;
        return -1;
    }
    
    fp->f_path = mount_root(fp->f_path);
    
    // Try to open 
    error = (*fp->f_ops->fo_open)(fp, flags);
    if (error) {
        errno = error;
        closef(fp);
        fdp->fd_ofiles[indx] = NULL;
        return -1;
    }
    
    return indx;
}
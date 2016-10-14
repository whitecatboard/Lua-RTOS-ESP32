/*
 * Whitecat, rename syscall implementation
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
#include "mount.h"

extern char *normalize_path(const char *path);

int __rename(const char *old_filename, const char *new_filename) {
    char *npathold;
    char *npathnew;
    struct file *fp;
    const char *device1;
    const char *device2;
    int res = 0;
    int fd;
    
    npathold = normalize_path(old_filename);
    if (!npathold) {
        errno = EIO;
        return -1;
    }
    
    npathnew = normalize_path(new_filename);
    if (!npathnew) {
        free(npathold);
        errno = EIO;
        return -1;
    }

    device1= mount_device(old_filename);
    device2= mount_device(new_filename);
    
    if (device1 == device2) {
        // Files are on same device
        if ((fd = open(old_filename, O_WRONLY)) == -1) {
            free(npathold);
            free(npathnew);
            return -1;
        }
        
        if (!(fp = getfile(fd))) {
            free(npathold);
            free(npathnew);
            close(fd);
            return -1;
        }

        npathold = mount_root(npathold);
        npathnew = mount_root(npathnew);
        
        res = (*fp->f_ops->fo_rename)(npathold, npathnew);    
        close(fd);
        if (res) {
            errno = res;
            res = -1;
        }
    } else {
        errno = EXDEV;
        res = -1;
    }
       
    free(npathold);
    free(npathnew);
    
    return res;    
}

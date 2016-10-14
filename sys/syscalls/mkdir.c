/*
 * Whitecat, mkdir sys call implementation
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

#include <sys/stat.h> 
#include <sys/types.h> 
#include <fcntl.h>

extern char *normalize_path(const char *path);

int mkdir(const char *path, mode_t mode) {
    const char *device;
    char *npath;
    int res;
    const struct devops *ops;
    
    npath = normalize_path(path);
    if (!npath) {
        return -1;
    }
    
    // Get device, and it's ops
    device = mount_device(npath);
    ops = getdevops(device);

    // Get root path
    npath = mount_root(npath);

    // Call op
    res = (*ops->fo_mkdir)(npath);  
    if (res) {
        errno = res;
        return -1;
    }
        
    return 0;    
}


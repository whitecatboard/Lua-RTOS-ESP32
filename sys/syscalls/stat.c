/*
 * Lua RTOS, stat, fstat syscall implementation
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

extern struct filedesc *p_fd;

int __fstat(struct _reent *r, int fd, struct stat *sb) {
    register struct file *fp;
    register struct filedesc *fdp = p_fd;
    int res;

    mtx_lock(&fd_mtx);
    if ((u_int)fd >= fdp->fd_nfiles ||
        (fp = fdp->fd_ofiles[fd]) == NULL) {
        mtx_unlock(&fd_mtx);
        __errno_r(r) = EBADF;
        return -1;
    }
    mtx_unlock(&fd_mtx);
    
    res = (fp->f_ops->fo_stat)(fp, sb);
    if (res > 0) {
        __errno_r(r) = res;
        return -1;
    }
    
    return (0);
}

int __stat(struct _reent *r, const char *str, struct stat *sb) {
    int fd, rv;

    fd = open(str, 0);
    if (fd < 0)
        return (-1);
    rv = __fstat(r, fd, sb);
    (void)close(fd);
    return (rv);
}

int lstat(const char *str, struct stat *sb) {
    return stat(str, sb);
}

#if ((PLATFORM_ESP32 != 1) && (PLATFORM_ESP8266 != 1))

int stat(const char *str, struct stat *sb) {
	return __stat(_GLOBAL_REENT, str, sb);
}

int fstat(int fd, struct stat *sb) {
	return __fstat(_GLOBAL_REENT, fd, sb);
}

#endif
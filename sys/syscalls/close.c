/*
 * Lua RTOS, write syscall implementation
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

static void munmapfd(int fd) {
    p_fd->fd_ofileflags[fd] &= ~UF_MAPPED;
}

int __close(struct _reent *r, int fd) {
    register struct filedesc *fdp = p_fd;
    register struct file *fp;
    register u_char *pf;
    int error;

    mtx_lock(&fd_mtx);
    if ((u_int)fd >= fdp->fd_nfiles ||
        (fp = fdp->fd_ofiles[fd]) == NULL) {
        mtx_unlock(&fd_mtx);
        __errno_r(r) = EBADF;
        return -1;
    }

    pf = (u_char *)&fdp->fd_ofileflags[fd];
    if (*pf & UF_MAPPED)
        (void) munmapfd(fd);
    fdp->fd_ofiles[fd] = NULL;
    while (fdp->fd_lastfile > 0 && fdp->fd_ofiles[fdp->fd_lastfile] == NULL)
        fdp->fd_lastfile--;
    if (fd < fdp->fd_freefile)
        fdp->fd_freefile = fd;
    *pf = 0;
    
    mtx_unlock(&fd_mtx);
    
    error = closef(fp);
    if (error) {
        __errno_r(r) = error;
        return -1;
    }
    
    return 0;
}

#if ((PLATFORM_ESP32 != 1) && (PLATFORM_ESP8266 != 1))

int close(int fd) {
	return __close(_GLOBAL_REENT, fd);	
}

#endif
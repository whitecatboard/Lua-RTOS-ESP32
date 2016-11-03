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

extern const struct filedesc *p_fd;

int __write(struct _reent *r, int fd, const void *buf, size_t nbyte) {
    register struct file *fp;
    const struct filedesc *fdp = p_fd;
    struct uio auio;
    struct iovec aiov;
    long cnt, error = 0;

    mtx_lock(&fd_mtx);
    if (((u_int)fd) >= fdp->fd_nfiles ||
        (fp = fdp->fd_ofiles[fd]) == NULL ||
        (fp->f_flag & FWRITE) == 0) {
        mtx_unlock(&fd_mtx);
        __errno_r(r) = EBADF;
        return -1;
    }
    mtx_unlock(&fd_mtx);

    aiov.iov_base = (caddr_t)buf;
    aiov.iov_len = nbyte;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_resid = nbyte;
    auio.uio_rw = UIO_WRITE;

    cnt = nbyte;

    error = (*fp->f_ops->fo_write)(fp, &auio);
    if (error) {
        if (auio.uio_resid != cnt && (error == ERESTART ||
            error == EINTR || error == EWOULDBLOCK))
            error = 0;
    }
    cnt -= auio.uio_resid;
    
    if (error) {
        __errno_r(r) = error;
        return -1;
    }
    
    return cnt;
}

#if ((PLATFORM_ESP32 != 1) && (PLATFORM_ESP8266 != 1))

int write(int fd, const void *buf, size_t nbyte) {
	return __write(_GLOBAL_REENT, fd, buf, nbyte);
}

#endif

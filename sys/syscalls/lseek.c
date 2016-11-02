/*
 * Lua RTOS, lseek syscall implementation
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

off_t __lseek(struct _reent *r, int fd, off_t offset, int whence) {
    register struct filedesc *fdp = p_fd;
    register struct file *fp;
    int error;

    mtx_lock(&fd_mtx);
    if ((u_int)fd >= fdp->fd_nfiles ||
        (fp = fdp->fd_ofiles[fd]) == NULL) {
        mtx_unlock(&fd_mtx);
        __errno_r(r) = EBADF;
        return -1;
    }
    mtx_unlock(&fd_mtx);

    if (fp->f_type != DTYPE_VNODE) {
        __errno_r(r) = ESPIPE;
        return -1;
    }
    
    switch (whence) {
        case L_INCR:
            fp->f_offset += offset;
            break;
        case L_XTND:
//            fp->f_offset = offset + ((FIL *)fp->f_fs)->fsize;              
            break;
        case L_SET:
            fp->f_offset = offset;
            break;
        default:
            __errno_r(r) = EINVAL;
            return -1;
    }
    
    error = (*fp->f_ops->fo_seek)(fp, offset, whence);
    if (error){
        __errno_r(r) = error;
        return -1;
    }

    return fp->f_offset;
}
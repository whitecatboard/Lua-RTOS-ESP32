/*
 * 
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 *
 * Copyright (c) 1982, 1986, 1989, 1993
 *  The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)sys_generic.c   8.9 (Berkeley) 2/14/95
 */

#ifndef DEVICE_H
#define	DEVICE_H

#include <limits.h>

#include <sys/uio.h>
#include <sys/file.h>

struct  devops {
    int (*fo_open) __P((struct file *fp, int flags));
    int (*fo_read)  __P((struct file *fp, struct uio *uio));
    int (*fo_write) __P((struct file *fp, struct uio *uio));
    int (*fo_ioctl) __P((struct file *fp, u_long com,
                        caddr_t data));
    int (*fo_select) __P((struct file *fp, int which));
    int (*fo_stat) __P((struct file *fp, struct stat *sb));
    int (*fo_close) __P((struct file *fp));
    int (*fo_opendir) __P((struct file *fp));
    int (*fo_readdir) __P((struct file *fp, struct dirent *ent));
    int (*fo_unlink) __P((struct file *fp));
    off_t (*fo_seek) __P((struct file *fp, off_t offset, int where));
    int (*fo_rename) __P((const char *old_filename, const char *new_filename));
    int (*fo_mkdir) __P((const char *pathname));
    int (*fo_format) ();
};
    
struct device {
    const char  *d_name;
    const struct devops d_ops;  
};

const struct devops *getdevops(const char *devname);

#endif	/* DEVICE_H */


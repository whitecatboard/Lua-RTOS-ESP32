/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *  The Regents of the University of California.  All rights reserved.
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
 *  @(#)file.h  8.3 (Berkeley) 1/9/95
 */

#ifndef __FILE_H
#define __FILE_H

#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#ifndef L_SET
#define L_SET   0       /* Seek from beginning of file.  */
#define L_INCR  1       /* Seek from current position.  */
#define L_XTND  2       /* Seek from end of file.  */
#endif

#ifdef KERNEL
#include <sys/queue.h>
#include <sys/sys/dirent.h>
  
/*
 * Kernel descriptor table.
 * One entry for each open kernel vnode and socket.
 */
struct file {
    LIST_ENTRY(file) f_list;/* list of active files */
    short   f_flag;         /* see fcntl.h */
#define DTYPE_VNODE     1   /* file */
#define DTYPE_SOCKET    2   /* communications endpoint */
    short   f_type;         /* descriptor type */
    short   f_count;        /* reference count */
    //short   f_msgcount;     /* references from message queue */
    struct  ucred *f_cred;  /* credentials associated with descriptor */
    struct  fileops {
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
    } *f_ops;
    off_t   f_offset;
    caddr_t f_data;         /* vnode or socket */
    void   *f_fs;
    void   *f_dir;
    char   *f_path;
    short   f_devunit;
    char   *f_devname;
};

LIST_HEAD(filelist, file);
extern struct filelist filehead;    /* head of list of open files */

#endif /* KERNEL */

#endif

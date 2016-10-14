/*
 * Whitecat, readdir implementation
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
 
#include <stdint.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/syscalls/mount.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/file.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern struct file *getfile(int fd);


/*
 * get next entry in a directory.
 */
struct dirent *
readdir(dirp)
	register DIR *dirp;
{
    register struct dirent *dp;
    struct file *fp;
    int res;
    
    if (!(fp = getfile(dirp->dd_fd))) {
        return NULL;
    }

    dp = (struct dirent *)dirp->dd_buf;

    if (dirp->mounted) {
        if (mount_readdir(fp->f_devname, fp->f_path, dirp->cmounted, dp->d_name)) {
            dp->d_namlen = strlen(dp->d_name);
            dp->d_type = DT_DIR;
            dp->d_reclen = 0;
            dirp->cmounted++;
            
            return dp;
        }
    }
    
    if ((res = (*fp->f_ops->fo_readdir)(fp, dp)) > 0) {
        errno = res;
        return NULL;
    }
    
    if (*dp->d_name) {
        return dp;    
    } else {
        return NULL;
    }
}

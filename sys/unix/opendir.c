/*
 * Whitecat, opendir implementation
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
#include <unistd.h>

extern struct file *getfile(int fd);
/*
 * Open a directory.
 */
DIR *opendir(const char *name) {
    struct stat statb;
    struct file *fp;
    DIR *dirp;
    int fd;
    int res;
    
    if ((fd = open(name, O_RDONLY)) == -1)
        return (NULL);

    if (fstat(fd, &statb) || !S_ISDIR(statb.st_mode)) {
        errno = ENOTDIR;
        close(fd);
        return (NULL);
    }

    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        close(fd);
        return (NULL);
    }

    if (!(fp = getfile(fd))) {
        close(fd);
        return NULL;
    }

    if ((res = (*fp->f_ops->fo_opendir)(fp)) == 0) {
        dirp = (DIR *)malloc(sizeof(DIR));
        if (!dirp) {
            errno = ENOMEM;
            return NULL;
        }  

        dirp->dd_len = 255;
        dirp->dd_buf = (char *)malloc(255);
        if (!dirp->dd_buf) {
            free(dirp);

            errno = ENOMEM;
            return NULL;            
        }
    } else {
        errno = res;
        return NULL;
    }


    dirp->dd_fd = fd;
    
    dirp->mounted = mount_dirs(fp->f_devname, fp->f_path);
    dirp->cmounted = 0;
    
    return (dirp);
}


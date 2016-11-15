/*
 * Lua RTOS, opendir implementation
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

#include <limits.h>

#include <stdint.h>
#include <sys/types.h>

#include <sys/cdefs.h>
#include <sys/mount.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/dirent.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <syscalls/syscalls.h>

/*
 * Open a directory.
 */
DIR *opendir(const char *name) {
	register DIR *dirp;
	register int fd;
	int rc = 0;
	char *fname;

	fname = mount_full_path(name);

	if ((fd = open(fname, 0)) == -1)
		return NULL;

	free(fname);

	rc = fcntl(fd & 0b111111111111, F_SETFD, 1);
	if (rc == -1 ||
	    (dirp = (DIR *)malloc(sizeof(DIR))) == NULL) {
		close (fd);
		return NULL;
	}
	/*
	 * If CLSIZE is an exact multiple of DIRBLKSIZ, use a CLSIZE
	 * buffer that it cluster boundary aligned.
	 * Hopefully this can be a big win someday by allowing page trades
	 * to user space to be done by getdirentries()
	 */
	dirp->dd_buf = malloc (512);
	dirp->dd_len = 512;

	if (dirp->dd_buf == NULL) {
		free (dirp);
		close (fd);
		return NULL;
	}

	dirp->dd_fd = fd;
	dirp->dd_loc = 0;
	dirp->dd_seek = 0;
	/*
	 * Set up seek point for rewinddir.
	 */

	return dirp;
}


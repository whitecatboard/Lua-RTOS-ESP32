/*
 * Lua RTOS, readdir implementation
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

#include <sys/dirent.h>
#include <syscalls/syscalls.h>

#include <stdio.h>

/*
 * get next entry in a directory.
 */
struct dirent *readdir(DIR *dirp) {
  register struct dirent *dp;

  for (;;) {
	if (dirp->dd_loc == 0) {
	  dirp->dd_size = getdents (dirp->dd_fd,
				dirp->dd_buf,
				dirp->dd_len);

	  if (dirp->dd_size <= 0) {
		return NULL;
	  }
	}

	if (dirp->dd_loc >= dirp->dd_size) {
	  dirp->dd_loc = 0;
	  continue;
	}

	dp = (struct dirent *)(dirp->dd_buf + dirp->dd_loc);
	if (dp->d_reclen <= 0 ||
	  dp->d_reclen > dirp->dd_len + 1 - dirp->dd_loc) {
	  return NULL;
	}

	dirp->dd_loc += dp->d_reclen;
	if (dp->d_ino == 0)
	  continue;
	return (dp);
  }

  return NULL;
}

/*
 * Lua RTOS, posix sockets wrappers
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

/*
 * This wrappers are required for allow lwip sockets to work in a vfs
 * (virtual file system). This is useful, for example, for open a file
 * stream over an opened socket and use stdio functions for write
 * networks apps (such as webservers, telnet servers, etc ...).
 *
 * The mission of this wrappers are to translate newlib file descriptors
 * to lwip file descriptors, and vice-versa. Remember that in esp-idf
 * vfs implementation a file descriptor is formed by 2 parts (the vfs
 * identifier and the file descriptor number). Example:
 *
 * lwip fd 0 it's (vfs_id | lwip fd) in newlib
 *
 */
#include "sdkconfig.h"

#include "lwip/sockets.h"

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

int esp_vfs_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);

#if 0
int __wrap_lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout) {
	int esp_vfs_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout)
#if 0
	// We need to convert lwip sets to newlib sets
	for(i=0;i<maxfdp;i++) {
		if (readset) {
			if (FD_ISSET(i, readset)) {
				FD_SET(socket_to_fd(vfs, i), readset);
			} else {
				FD_CLR(socket_to_fd(vfs, i), readset);
			}
		}

		if (writeset) {
			if (FD_ISSET(i, writeset)) {
				FD_SET(vfs | socket_to_fd(vfs,i), writeset);
			} else {
				FD_CLR(socket_to_fd(vfs, i), writeset);
			}
		}

		if (exceptset) {
			if (FD_ISSET(i, exceptset)) {
				FD_SET(socket_to_fd(vfs, i), exceptset);
			} else {
				FD_CLR(socket_to_fd(vfs, i), exceptset);
			}
		}
	}
#endif

}

#endif

//int select(int maxfdp1,fd_set *readset,fd_set *writeset,fd_set *exceptset,struct timeval *timeout) {
//	return esp_vfs_select(maxfdp1, readset, writeset, exceptset, timeout);
//}


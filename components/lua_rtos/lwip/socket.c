/*
 * Lua RTOS, posix sockets wrappers
 *
 * Copyright (C) 2015 - 2017
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

#include "luartos.h"

#include "lwip/sockets.h"

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#if USE_NET_VFS
#if 0
int _curl_socket = 0;
#endif
#define fd_to_socket(fd) (fd & ((1 << CONFIG_MAX_FD_BITS) - 1))
#else
#define fd_to_socket(fd) fd
#endif

#define socket_to_fd(vfs, fd) (vfs | fd)

extern int __real_lwip_accept_r(int s, struct sockaddr *addr, socklen_t *addrlen);
extern int __real_lwip_bind_r(int s, const struct sockaddr *name, socklen_t namelen);
extern int __real_lwip_shutdown_r(int s, int how);
extern int __real_lwip_getpeername_r(int s, struct sockaddr *name, socklen_t *namelen);
extern int __real_lwip_getsockname_r(int s, struct sockaddr *name, socklen_t *namelen);
extern int __real_lwip_getsockopt_r(int s, int level, int optname, void *optval, socklen_t *optlen);
extern int __real_lwip_setsockopt_r(int s, int level, int optname, const void *optval, socklen_t optlen);
extern int __real_lwip_connect_r(int s, const struct sockaddr *name, socklen_t namelen);
extern int __real_lwip_listen_r(int s, int backlog);
extern int __real_lwip_recv_r(int s, void *mem, size_t len, int flags);
extern int __real_lwip_read_r(int s, void *mem, size_t len);
extern int __real_lwip_recvfrom_r(int s, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
extern int __real_lwip_send_r(int s, const void *dataptr, size_t size, int flags);
extern int __real_lwip_sendmsg_r(int s, const struct msghdr *message, int flags);
extern int __real_lwip_sendto_r(int s, const void *dataptr, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);
extern int __real_lwip_socket(int domain, int type, int protocol);
extern int __real_lwip_write_r(int s, const void *dataptr, size_t size);
extern int __real_lwip_writev_r(int s, const struct iovec *iov, int iovcnt);
extern int __real_lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
extern int __real_lwip_ioctl_r(int s, long cmd, void *argp);
extern int __real_lwip_fcntl_r(int s, int cmd, int val);
extern int __real_lwip_close_r(int s);

int __wrap_lwip_accept_r(int fd, struct sockaddr *addr, socklen_t *addrlen) {
	int s = __real_lwip_accept_r(fd_to_socket(fd), addr, addrlen);

#if USE_NET_VFS
	if (s != -1) {
		char device[15];

		snprintf(device, sizeof(device), "/dev/socket/%d", s);
		FILE *stream = fopen(device,"a+");
		if (stream) {
			return stream->_file;
		} else {
			return -1;
		}
	}
#endif

	return s;
}

int __wrap_lwip_bind_r(int fd, const struct sockaddr *name, socklen_t namelen) {
	return __real_lwip_bind_r(fd_to_socket(fd), name, namelen);
}

int __wrap_lwip_shutdown_r(int fd, int how) {
	return __real_lwip_shutdown_r(fd_to_socket(fd), how);
}

int __wrap_lwip_getpeername_r(int fd, struct sockaddr *name, socklen_t *namelen) {
	return __real_lwip_getpeername_r(fd_to_socket(fd), name, namelen);
}

int __wrap_lwip_getsockname_r(int fd, struct sockaddr *name, socklen_t *namelen) {
	return __real_lwip_getsockname_r(fd_to_socket(fd), name, namelen);
}

int __wrap_lwip_getsockopt_r(int fd, int level, int optname, void *optval, socklen_t *optlen) {
	return __real_lwip_getsockopt_r(fd_to_socket(fd), level, optname, optval, optlen);
}

int __wrap_lwip_setsockopt_r(int fd, int level, int optname, const void *optval, socklen_t optlen) {
	return __real_lwip_setsockopt_r(fd_to_socket(fd), level, optname, optval, optlen);
}

int __wrap_lwip_connect_r(int fd, const struct sockaddr *name, socklen_t namelen) {
	return __real_lwip_connect_r(fd_to_socket(fd), name, namelen);
}

int __wrap_lwip_listen_r(int fd, int backlog) {
	return __real_lwip_listen_r(fd_to_socket(fd), backlog);
}

int __wrap_lwip_recv_r(int fd, void *mem, size_t len, int flags) {
	return __real_lwip_recv_r(fd_to_socket(fd), mem, len, flags);
}

int __wrap_lwip_read_r(int fd, void *mem, size_t len) {
	return __real_lwip_read_r(fd_to_socket(fd), mem, len);
}

int __wrap_lwip_recvfrom_r(int fd, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen) {
	return __real_lwip_recvfrom_r(fd_to_socket(fd), mem, len, flags, from, fromlen);
}

int __wrap_lwip_send_r(int fd, const void *dataptr, size_t size, int flags) {
	return __real_lwip_send_r(fd_to_socket(fd), dataptr, size, flags);
}

int __wrap_lwip_sendmsg_r(int fd, const struct msghdr *message, int flags) {
	return __real_lwip_sendmsg_r(fd_to_socket(fd), message, flags);
}

int __wrap_lwip_sendto_r(int fd, const void *dataptr, size_t size, int flags, const struct sockaddr *to, socklen_t tolen) {
	return __real_lwip_sendto_r(fd_to_socket(fd), dataptr, size, flags, to, tolen);
}
int __wrap_lwip_write_r(int fd, const void *dataptr, size_t size) {
	return __real_lwip_write_r(fd_to_socket(fd), dataptr, size);
}

int __wrap_lwip_writev_r(int fd, const struct iovec *iov, int iovcnt) {
	return __real_lwip_writev_r(fd_to_socket(fd), iov, iovcnt);
}

int __wrap_lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout) {
#if USE_NET_VFS
	int vfs = maxfdp1 & (((1 << (16 - CONFIG_MAX_FD_BITS - 1)) - 1) << CONFIG_MAX_FD_BITS);
	int maxfdp = ((1 << CONFIG_MAX_FD_BITS) - 1);
	int i, s;

    // Convert input fd sets to internal lwip sets
    for(i=0;i<maxfdp1;i++) {
    	if (readset) {
    		if (FD_ISSET(i, readset)) {
    			FD_SET(fd_to_socket(i), readset);
    		} else {
    			FD_CLR(fd_to_socket(i), readset);
    		}
    	}

    	if (writeset) {
    		if (FD_ISSET(i, writeset)) {
    			FD_SET(fd_to_socket(i), writeset);
    		} else {
    			FD_CLR(fd_to_socket(i), writeset);
    		}
    	}

    	if (exceptset) {
    		if (FD_ISSET(i, exceptset)) {
    			FD_SET(fd_to_socket(i), exceptset);
    		} else {
    			FD_CLR(fd_to_socket(i), exceptset);
    		}
    	}
    }

    s = __real_lwip_select(maxfdp, readset, writeset, exceptset, timeout);

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

    return s;
#else
    return __real_lwip_select(maxfdp1, readset, writeset, exceptset, timeout);
#endif
}

int __wrap_lwip_ioctl_r(int fd, long cmd, void *argp) {
	return __real_lwip_ioctl_r(fd_to_socket(fd), cmd, argp);
}

int __wrap_lwip_fcntl_r(int fd, int cmd, int val) {
	return __real_lwip_fcntl_r(fd_to_socket(fd), cmd, val);
}

int __wrap_lwip_socket(int domain, int type, int protocol) {
	int s = __real_lwip_socket(domain, type, protocol);

#if USE_NET_VFS
#if 0
	if ((s != -1) && (!_curl_socket)) {
#else
	if (s != -1) {
#endif
		char device[15];

		snprintf(device, sizeof(device), "/dev/socket/%d", s);
		FILE *stream = fopen(device,"a+");
		if (stream) {
			return stream->_file;
		} else {
			return -1;
		}
	}
#endif

	return s;
}

int __wrap_lwip_close_r(int fd) {
	return __real_lwip_close_r(fd_to_socket(fd));
}

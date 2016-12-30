#include "luartos.h"

#include "lwip/sockets.h"

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include <syscalls/file.h>
#include <syscalls/filedesc.h>

extern struct file *get_file(int fd);

#if USE_NET_VFS

static int fd_to_socket(int fd) {
	struct file *fp;

	// Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

	if (fp->f_fs_type != 4) {
		errno = EBADF;
		return -1;
	}

	return fp->unit;
}
#else
#define fd_to_socket(fd) fd
#endif

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
extern int __real_lwip_select_r(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
extern int __real_lwip_ioctl_r(int s, long cmd, void *argp);
extern int __real_lwip_fcntl_r(int s, int cmd, int val);

int __wrap_lwip_accept_r(int fd, struct sockaddr *addr, socklen_t *addrlen) {
	int s;

	s = __real_lwip_accept_r(fd_to_socket(fd), addr, addrlen);

#if USE_NET_VFS
	if (s != -1) {
		char device[15];

		snprintf(device, sizeof(device), "/dev/socket/%d", s);
		FILE *stream = fopen(device,"a+");
		if (stream) {
			return stream->_file;
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

int __wrap_lwip_select_r(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout) {
	int s;

	s = __real_lwip_select_r(maxfdp1, readset, writeset, exceptset, timeout);

	return s;
}

int __wrap_lwip_ioctl_r(int fd, long cmd, void *argp) {
	return __real_lwip_ioctl_r(fd_to_socket(fd), cmd, argp);
}

int __wrap_lwip_fcntl_r(int fd, int cmd, int val) {
	return __real_lwip_fcntl_r(fd_to_socket(fd), cmd, val);
}

int __wrap_lwip_socket(int domain, int type, int protocol) {
	int s;

	s = __real_lwip_socket(domain, type, protocol);

#if USE_NET_VFS
	if (s != -1) {
		char device[15];

		snprintf(device, sizeof(device), "/dev/socket/%d", s);
		FILE *stream = fopen(device,"a+");
		if (stream) {
			return stream->_file;
		}
	}
#endif

	return s;
}

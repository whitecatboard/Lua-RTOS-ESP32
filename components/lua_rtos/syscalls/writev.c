#include <sys/types.h>
#include <lwip/sockets.h>

ssize_t esp_vfs_writev(int fd, const struct iovec *iov, int iovcnt);

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
	return esp_vfs_writev(fd, iov, iovcnt);
}

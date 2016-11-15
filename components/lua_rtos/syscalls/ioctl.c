#include <errno.h>
#include <stdarg.h>

#include <syscalls/syscalls.h>

int vfs_tty_ioctl(int fd, unsigned long request, ...);

int ioctl(int fd, unsigned long request, ...) {
	struct file *fp;
	int res = 0;

	// Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

    va_list args;
    va_start(args, request);

    if (fp->f_fs_type == FS_TTY) {
    	res = vfs_tty_ioctl(fd, request, args);
	}

    va_end(args);

    return res;
}

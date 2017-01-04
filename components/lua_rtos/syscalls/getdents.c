#include "luartos.h"

#include <errno.h>

#include <syscalls/syscalls.h>

#include <sys/dirent.h>

extern int vfs_spiffs_getdents(int fd, void *buff, int size);
extern int vfs_fat_getdents(int fd, void *buff, int size);

int getdents (int fd, void *buff, int size) {
	struct file *fp;

	// Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

#if USE_SPIFFS
	if (fp->f_fs_type == FS_SPIFFS) {
		return vfs_spiffs_getdents(fd, buff, size);
	}
#endif

#if USE_FAT
	if (fp->f_fs_type == FS_FAT) {
		return vfs_fat_getdents(fd, buff, size);
	}
#endif

	return -1;
}

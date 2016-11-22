#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/dirent.h>
#include <sys/sys_dirent.h>

int closedir(DIR *dirp) {
	int fd;

	fd = dirp->dd_fd;

	free(dirp->dd_buf);
	free(dirp);

	return(close(fd));
}

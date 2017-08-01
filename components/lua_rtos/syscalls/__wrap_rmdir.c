#include "esp_attr.h"

#include <limits.h>
#include <reent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/mount.h>

extern int __real_rmdir(const char* path);

int __wrap_rmdir(const char* path) {
	char *ppath;
	int res;

	if (!path || !*path) {
		errno = ENOENT;
		return -1;
	}

	ppath = mount_resolve_to_physical(path);
	if (ppath) {
		res = __real_rmdir(ppath);

		free(ppath);

		return res;
	} else {
		return -1;
	}
}

#include "esp_attr.h"

#include <limits.h>
#include <reent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <dirent.h>

#include <sys/mount.h>

DIR* __real_readdir(const char* name);

DIR* __wrap_readdir(const char* name) {
	char *ppath;
	DIR *dir;

	if (!name || !*name) {
		errno = ENOENT;
		return (DIR*)-1;
	}

	ppath = mount_resolve_to_physical(name);
	if (ppath) {
		dir = __real_readdir(ppath);

		free(ppath);

		return dir;
	} else {
		return (DIR*)-1;
	}
}

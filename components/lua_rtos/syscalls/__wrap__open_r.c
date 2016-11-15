#include "esp_attr.h"

#include <limits.h>
#include <reent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/mount.h>

extern int __real__open_r(struct _reent *r, const char *path, int flags, int mode);

int IRAM_ATTR __wrap__open_r(struct _reent *r, const char *path, int flags, int mode) {
	char *fpath;
	int res;

	if ((strncmp(path,"/dev/uart/",10) == 0) || (strncmp(path,"/dev/tty/",9) == 0)) {
		return __real__open_r(r, path, flags, mode);
	} else {
		fpath = mount_full_path(path);

		res = __real__open_r(r, fpath, flags, mode);

		if (path != fpath) free(fpath);

		return res;
	}
}

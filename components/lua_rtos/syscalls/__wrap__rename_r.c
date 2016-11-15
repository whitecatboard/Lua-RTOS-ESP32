#include "esp_attr.h"

#include <limits.h>
#include <reent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/mount.h>

extern int __real__rename_r(struct _reent *r, const char *src, const char *dst);

int IRAM_ATTR __wrap__rename_r(struct _reent *r, const char *src, const char *dst) {
	char *fpath_src;
	char *fpath_dst;
	int res;

	fpath_src = mount_full_path(src);
	fpath_dst = mount_full_path(dst);

	res = __real__rename_r(r, fpath_src, fpath_dst);

	if (src != fpath_src) free(fpath_src);
	if (src != fpath_dst) free(fpath_dst);

	return res;
}

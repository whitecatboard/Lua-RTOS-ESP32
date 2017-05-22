#include "esp_attr.h"

#include <limits.h>
#include <reent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/mount.h>

extern int __real__rename_r(struct _reent *r, const char *src, const char *dst);

int IRAM_ATTR __wrap__rename_r(struct _reent *r, const char *src, const char *dst) {
	char *ppath_src;
	char *ppath_dst;
	int res;

	if (!src || !*src) {
		errno = ENOENT;
		return -1;
	}

	if (!dst || !*dst) {
		errno = ENOENT;
		return -1;
	}


	ppath_src = mount_resolve_to_physical(src);
	if (!ppath_src) {
		return -1;
	}

	ppath_dst = mount_resolve_to_physical(dst);
	if (!ppath_dst) {
		free(ppath_src);
		return -1;
	}

	res = __real__rename_r(r, ppath_src, ppath_dst);

	free(ppath_src);
	free(ppath_dst);

	return res;
}

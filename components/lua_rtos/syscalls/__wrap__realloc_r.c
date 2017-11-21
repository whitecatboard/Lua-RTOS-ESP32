#include "freertos/adds.h"
#include "freertos/task.h"

#include "esp_attr.h"

#include <limits.h>
#include <reent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/mount.h>

#include <Lua/src/lgc.h>

extern int __real__realloc_r(struct _reent *r, void *ptr, size_t size);

int IRAM_ATTR __wrap__realloc_r(struct _reent *r, void *ptr, size_t size) {
	int res;

	if (!(res = __real__realloc_r(r, ptr,size))) {
		// Not enough memory
		// Perform a garbage collector, and try to free some memory
		lua_State *L = pvGetLuaState();

		if (L) {
			luaC_fullgc(L, 0);
			vTaskDelay(1 / portTICK_PERIOD_MS);
			luaC_fullgc(L, 0);
			vTaskDelay(1 / portTICK_PERIOD_MS);
			res = __real__realloc_r(r, ptr, size);
		}
	}

	return res;
}

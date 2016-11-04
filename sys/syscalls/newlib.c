/*
 * Lua RTOS, newlib stubs
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "syscalls.h"
#include "lua.h"

#include <sys/reent.h>
#include <sys/status.h>
#include <sys/drivers/clock.h>
#include <sys/drivers/uart.h>

extern void luaC_fullgc (lua_State *L, int isemergency);

#undef  RAM_FUNC_ATTR
#define RAM_FUNC_ATTR

#if PLATFORM_ESP32
#include "esp_attr.h"

#undef  RAM_FUNC_ATTR
#define RAM_FUNC_ATTR IRAM_ATTR
#endif

#if PLATFORM_ESP8266
#undef  RAM_FUNC_ATTR
#define RAM_FUNC_ATTR IRAM
#endif

#if !PLATFORM_ESP8266
void* RAM_FUNC_ATTR __malloc_r(struct _reent *r, size_t size) {
    return pvPortMalloc(size);
}

void RAM_FUNC_ATTR __free_r(struct _reent *r, void* ptr) {
    vPortFree(ptr);
}

void* RAM_FUNC_ATTR __realloc_r(struct _reent *r, void* ptr, size_t size) {
    void *new_chunk;

    if (size == 0) {
        if (ptr) {
            vPortFree(ptr);
        }
        return NULL;
    }

    new_chunk = pvPortMalloc(size);
    if (new_chunk && ptr) {
    	uint8_t *src = (uint8_t *)ptr;
    	uint8_t *dst = (uint8_t *)new_chunk;

    	while (size) {
    		*dst++ = *src++;
    		size--;
    	}

    	vPortFree(ptr);
    }

    return new_chunk;
}

void* RAM_FUNC_ATTR __calloc_r(struct _reent *r, size_t count, size_t size) {
    void *result = pvPortMalloc(count * size);
    if (result) {
    	uint8_t *src = (uint8_t *)result;

    	size = count * size;
    	while (size) {
    		*src++ = 0x00;
    		size--;
    	}
    }
    return result;
}
#else
// WRAP for malloc
//
// If memory cannot be allocated, launch Lua garbage collector and
// try again with the hope that then more memory will be available
extern void* __real_malloc(size_t bytes);
void* __wrap_malloc(size_t bytes) {
	void *ptr = __real_malloc(bytes);

	if (!ptr) {
		lua_State *L = pvGetLuaState();

		if (L) {
			lua_lock(L);
			luaC_fullgc(L, 1);
			lua_unlock(L);

			ptr = __real_malloc(bytes);
		}
	}

	return ptr;
}

extern void* __real_calloc(size_t n, size_t bytes);
void* __wrap_calloc(size_t n, size_t bytes) {
	void *ptr = __real_calloc(n, bytes);

	if (!ptr) {
		lua_State *L = pvGetLuaState();

		if (L) {
			lua_lock(L);
			luaC_fullgc(L, 1);
			lua_unlock(L);

			ptr = __real_calloc(n, bytes);
		}
	}

	return ptr;
}

extern void* __real_realloc(void *ptr, size_t bytes);
void* __wrap_realloc(void *ptr, size_t bytes) {
	void *nptr = __real_realloc(ptr, bytes);

	if (!nptr) {
		lua_State *L = pvGetLuaState();

		if (L) {
			lua_lock(L);
			luaC_fullgc(L, 1);
			lua_unlock(L);

			nptr = __real_realloc(ptr, bytes);
		}
	}

	return nptr;
}

extern uint32_t _irom0_text_start;
extern uint32_t _irom0_text_end;

extern void __real_free(void *ptr);
void __wrap_free(void *ptr) {
	__real_free(ptr);
}
#endif

#if !PLATFORM_ESP32
int _isatty_r(struct _reent *r, int fd) {
	return (fd < 3);
}

int isatty(int fd) {
	return _isatty_r(_GLOBAL_REENT, fd);
}
#endif

int __open_r(struct _reent *r, const char *pathname, int flags, int mode) {
	if (status_get(STATUS_SYSCALLS_INITED)) {
		return __open(r, pathname, flags, mode);
	}

	return 0;
}

int __close_r(struct _reent *r, int fd) {
	return __close(r, fd);
}

int  __unlink_r(struct _reent *r, const char *path) {
	return __unlink(r, path);
}

int __remove_r(struct _reent *r, const char *_path) {
	return (__unlink(r, _path));
}

int __rename_r(struct _reent *r, const char *_old, const char *_new) {
	return __rename(r, _old, _new);
}

int __fstat_r(struct _reent *r, int fd, struct stat * st) {
	if (status_get(STATUS_SYSCALLS_INITED)) {
		return __fstat(r, fd, st);		
	}

	st->st_mode = S_IFCHR;
    return 0;	
}	

int __stat_r(struct _reent *r, const char * path, struct stat * st) {
	return __stat(r, path, st);
}

_off_t __lseek_r(struct _reent *r, int fd, _off_t size, int mode) {
	return __lseek(r, fd, size, mode);
}

ssize_t __read_r(struct _reent *r, int fd, void *buf, size_t nbyte) {
	if (status_get(STATUS_SYSCALLS_INITED)) {
		return __read(r, fd, buf, nbyte);		
	}

	return nbyte;
}

ssize_t __write_r(struct _reent *r, int fd, const void *buf, size_t nbyte) {	
	if (status_get(STATUS_SYSCALLS_INITED)) {
		return __write(r, fd, buf, nbyte);		
	} else {
		int i = 0;
		
		while (i < nbyte) {
			uart_write(1, *((char *)buf++));
			i++;
		}		
	}

	return nbyte;
}

pid_t __getpid_r(struct _reent *r) {
	return __getpid(r);
}

clock_t __times_r(struct _reent *r, struct tms *ptms) {
	return __times(r, ptms);
}

int __gettimeofday_r(struct _reent *r, struct timeval *tv, void *tz) {
	return __gettimeofday(r, tv, tz);
}

unsigned sleep(unsigned int secs) {
    vTaskDelay( (secs * 1000) / ((TickType_t) 1000 / configTICK_RATE_HZ));

    return 0;
}

int usleep(useconds_t usec) {
	vTaskDelay(usec / ((TickType_t) 1000000 / configTICK_RATE_HZ));

	return 0;
}

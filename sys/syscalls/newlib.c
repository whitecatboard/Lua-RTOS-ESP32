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

#if !PLATFORM_ESP32

// If memory cannot be allocated, launch Lua garbage collector and
// try again with the hope that then more memory will be available
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

extern void __real_free(void *ptr);
void __wrap_free(void *ptr) {
	__real_free(ptr);
}

extern void *xPortSupervisorStackPointer;
static char *heap_end = NULL;
    
void *_sbrk_r (struct _reent *r, ptrdiff_t incr) {
    extern char   _heap_start; /* linker script defined */
	register int  stackptr asm("sp"); 
    char 		 *prev_heap_end;

    if (heap_end == NULL)
		heap_end = &_heap_start;
		
    prev_heap_end = heap_end;

    intptr_t sp = (intptr_t)xPortSupervisorStackPointer;
    if(sp == 0) /* scheduler not started */
        sp = stackptr;

    if ((intptr_t)heap_end + incr >= sp) {
        r->_errno = ENOMEM;
        return (caddr_t)-1;
    }

    heap_end += incr;

    return (caddr_t) prev_heap_end;
}

void *sbrk(int incr) {
	return (void *)_sbrk_r (_GLOBAL_REENT, incr);
}

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

void _newlib_init() {
}

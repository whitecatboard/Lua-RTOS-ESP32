#include "FreeRTOS.h"
#include "task.h"
#include "syscalls.h"
#include <stdio.h>
#include <sys/reent.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/status.h>

#include <sys/drivers/clock.h>
#include <sys/drivers/uart.h>

extern int __rename(const char *old_filename, const char *new_filename);
extern pid_t getpid(void);
extern void luaC_fullgc (lua_State *L, int isemergency);

extern int read(int fd, void *buf, size_t nbyte);
extern int write(int fd, const void *buf, size_t nbyte);

void vPortFree(void *pv);
void *pvPortMalloc(size_t xWantedSize);
void *pvPortRealloc(void *pv, size_t size);
long long ticks();

int __open(const char *path, int flags, ...);
int __close(int fd);
int __unlink(const char *path);
int __unlink(const char *path);
int __fstat(int fd, struct stat *sb);
int _stat_r(struct _reent *r, const char *pathname, void *buf);
off_t __lseek(int fd, off_t offset, int whence);
int __read(int fd, void *buf, size_t nbyte);

clock_t _clock_(void) {
	return (clock_t)ticks();
}

int __remove_r(struct _reent *r, const char *_path) {
	return (unlink(_path));
}

int _rename_r(struct _reent *r, const char *_old, const char *_new) {
	return __rename(_old, _new);
}

pid_t _getpid_r(struct _reent *r) {
	return getpid();
}

int _kill_r(struct _reent *r, int pid, int sig) {
	return 0;
}

clock_t _times_r(struct _reent *r, struct tms *ptms) {
	return ticks();
}

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

unsigned sleep(unsigned int secs) {
    vTaskDelay( (secs * 1000) / portTICK_PERIOD_MS );
    
    return 0;
}

int usleep(useconds_t usec) {
	vTaskDelay(usec / portTICK_PERIOD_US);
	
	return 0;
}
 
int _open_r(struct _reent *r, const char *pathname, int flags, int mode) {
	return open(pathname, flags, mode);
}

int _close_r(struct _reent *r, int fd) {
	return close(fd);
}

int _unlink_r(struct _reent *r, const char *path) {
	return unlink(path);
}

int _fstat_r(struct _reent *r, int fd, void *buf) {
	return fstat(fd, buf);
}	

int _stat_r(struct _reent *r, const char *pathname, void *buf) {
	return stat(pathname, buf);
}

off_t _lseek_r(struct _reent *r, int fd, off_t offset, int whence) {
	return lseek(fd, offset, whence);
}

int _write_r(struct _reent *r, int fd, const void *buf, size_t nbyte) {
	if (status_get(STATUS_SYSCALLS_INITED)) {
		return write(fd, buf, nbyte);		
	} else {
		int i = 0;
		
		while (i < nbyte) {
			uart_write(1, *((char *)buf++));
			i++;
		}		
	}

	return nbyte;
}

int _read_r(struct _reent *r, int fd, void *buf, size_t nbyte) {
	if (status_get(STATUS_SYSCALLS_INITED)) {
		return read(fd, buf, nbyte);		
	}
	
	return nbyte;
}

int _isatty_r(struct _reent *r, int fd) {
	return (fd < 3);
} 

int isatty(int fd) {
	return _isatty_r(_REENT, fd);
}
	
#ifdef PLATFORM_PIC32MZ
#include <malloc.h>

extern void *xPortSupervisorStackPointer;
static char *heap_end = NULL;
    
caddr_t _sbrk_r (struct _reent *r, int incr) {
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
	return (void *)_sbrk_r (_REENT, incr);
}

#endif
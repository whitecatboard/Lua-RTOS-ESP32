/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS pthread implementation for FreeRTOS
 *
 */

#include "luartos.h"

#ifndef __PTHREAD_H
#define	__PTHREAD_H

#include "freertos/FreeRTOS.h"
#include "freertos/adds.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <pthread.h>

#include <sys/mutex.h>
#include <sys/list.h>
#include <sys/time.h>

#include <signal.h>


#define CPU_INITIALIZER 0

// Add CPU cpu to set
#define CPU_SET(ncpu, cpuset) \
	*(cpuset) |= (1 << ncpu)

// Test to see if CPU cpu is a member of set.
#define CPU_ISSET(ncpu, cpuset) \
	(*(cpuset) & (1 << ncpu))

// Each thread maintains a signal handler copy. Typically there are around 32 defined
// signals, but not signals are required for applications. For example, in Lua only
// SIGINT is used.
//
// This defines how many signals will be available in threads
#define PTHREAD_NSIG      (SIGINT + 1)

// 1 for activate debug log when thread can't lock a mutex
#define PTHREAD_MTX_DEBUG 0

#if PTHREAD_MTX_DEBUG
#define PTHREAD_MTX_LOCK_TIMEOUT (3000 / portTICK_PERIOD_MS)
#define PTHREAD_MTX_DEBUG_LOCK() printf("phread can't lock\n");
#else
#define PTHREAD_MTX_LOCK_TIMEOUT portMAX_DELAY
#define PTHREAD_MTX_DEBUG_LOCK()
#endif

// Minimal stack size per thread
#define PTHREAD_STACK_MIN (1024 * 2)

#define PTHREAD_CREATE_DETACHED 0

// Initial states for a thread
#define PTHREAD_INITIAL_STATE_RUN     1
#define PTHREAD_INITIAL_STATE_SUSPEND 2

//#define PTHREAD_ONCE_INIT             {NULL}

// Required structures and types

struct pthread_mutex {
    SemaphoreHandle_t sem;
    int owner;
    int type;
};

struct pthread_cond {
    struct mtx mutex;
    int referenced;
};

struct pthread_once {
    struct mtx mutex;
};

struct pthread_key_specific {
    pthread_t thread;
    const void *value;
};

struct pthread_key {
    struct list specific;
    void (*destructor)(void*);
};

struct pthread_join {
    QueueHandle_t queue;
};

struct pthread_clean {
    void (*clean)(void*);
    void *args;
};

struct pthread {
    struct list join_list;
    struct list clean_list;
    struct mtx init_mtx;
    sig_t signals[PTHREAD_NSIG];
    pthread_t thread;
    xTaskHandle task;
};

struct pthread_attr {
    int stack_size;
    int initial_state;
    int sched_priority;
    int cpuset;
};

//struct sched_param {
//    int sched_priority;
//};


// Helper functions, only for internal use
void  _pthread_init();
int   _pthread_create(pthread_t *id, int priority, int stacksize, int cpu, int initial_state, void *(*start_routine)(void *), void *args);
int   _pthread_join(pthread_t id);
int   _pthread_free(pthread_t id);
sig_t _pthread_signal(int s, sig_t h);
void  _pthread_exec_signal(int dst, int s);
void  _pthread_process_signal();
int   _pthread_has_signal(int dst, int s);
int   _pthread_stop(pthread_t id);
int   _pthread_suspend(pthread_t id);
int   _pthread_resume(pthread_t id);
int   _pthread_core(pthread_t id);
sig_t _pthread_signal(int s, sig_t h);
int   _pthread_get_prio();
int   _pthread_stack_free(pthread_t id);
int   _pthread_stack(pthread_t id);
struct pthread *_pthread_get(pthread_t id);

// API functions
int  pthread_attr_init(pthread_attr_t *attr);
int  pthread_attr_destroy(pthread_attr_t *attr);
int  pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int  pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int  pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param);
int  pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param);
int  pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int  pthread_attr_setinitialstate(pthread_attr_t *attr, int initial_state);
int  pthread_attr_setaffinity_np(pthread_attr_t *attr, size_t cpusetsize, const cpu_set_t *cpuset);
int  pthread_attr_getaffinity_np(const pthread_attr_t *attr, size_t cpusetsize, cpu_set_t *cpuset);

int  pthread_mutex_init(pthread_mutex_t *mut, const pthread_mutexattr_t *attr);
int  pthread_mutex_lock(pthread_mutex_t *mut);
int  pthread_mutex_unlock(pthread_mutex_t *mut);
int  pthread_mutex_trylock(pthread_mutex_t *mut);
int  pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int  pthread_mutexattr_init(pthread_mutexattr_t *attr);
int  pthread_mutex_destroy(pthread_mutex_t *mutex);

int  pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int  pthread_cond_destroy(pthread_cond_t *cond);
int  pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int  pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);

int  pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
int  pthread_setcancelstate(int state, int *oldstate);
int  pthread_key_create(pthread_key_t *k, void (*destructor)(void*));
int  pthread_setspecific(pthread_key_t k, const void *value);
void *pthread_getspecific(pthread_key_t k);
int  pthread_join(pthread_t thread, void **value_ptr);
int pthread_cancel(pthread_t thread);
int pthread_kill(pthread_t thread, int signal);

int pthread_setname_np(pthread_t id, const char *name);
int pthread_getname_np(pthread_t id, char *name, size_t len);

pthread_t pthread_self(void);

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *args);

void pthread_cleanup_push(void (*routine)(void *), void *arg);

#endif	/* __PTHREAD_H */


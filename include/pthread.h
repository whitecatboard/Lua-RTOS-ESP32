/*
 * Lua RTOS, pthread implementation over FreeRTOS
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

#ifndef PTHREAD_H
#define	PTHREAD_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <sys/mutex.h>
#include <sys/list/list.h>
#include <sys/time.h>
#include <signal.h>

#define PTHREAD_NSIG 4

#define PTHREAD_MTX_DEBUG 0

#if PTHREAD_MTX_DEBUG
#define PTHREAD_MTX_LOCK_TIMEOUT (3000 / portTICK_PERIOD_MS)
#define PTHREAD_MTX_DEBUG_LOCK() printf("phread can't lock\n");
#else
#define PTHREAD_MTX_LOCK_TIMEOUT portMAX_DELAY
#define PTHREAD_MTX_DEBUG_LOCK() 
#endif

#define PTHREAD_STACK_MIN configMINIMAL_STACK_SIZE

#define PTHREAD_CANCEL_DISABLE 1

#define PTHREAD_MIN       1
#define PTHREAD_MAX       PTHREAD_MIN + 10

#define PTHREAD_MUTEX_MIN 1
#define PTHREAD_MUTEX_MAX PTHREAD_MUTEX_MIN + 10

struct pthread_mutex_attr {
    int type;
};

typedef struct pthread_mutex_attr pthread_mutexattr_t;


struct pthread_mutex {
    SemaphoreHandle_t sem;
    int owner;
    int type;
};

typedef unsigned int pthread_mutex_t;
typedef unsigned int pthread_condattr_t;
typedef int pthread_t;
typedef int pthread_key_t;

struct pthread_cond {
    struct mtx mutex;
};

typedef struct pthread_cond pthread_cond_t;

struct pthread_once {
    struct mtx mutex;
};

typedef struct pthread_once pthread_once_t;

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
};

typedef struct pthread_attr pthread_attr_t;

#define PTHREAD_INITIAL_STATE_RUN 1
#define PTHREAD_INITIAL_STATE_SUSPEND 2

#define PTHREAD_CREATE_DETACHED 1
#define PTHREAD_CREATE_JOINABLE 2

void _pthread_init();
int _pthread_create(pthread_t *id, int stacksize, int initial_state, void *(*start_routine)(void *), void *args);
int _pthread_join(pthread_t id);
int _pthread_free(pthread_t id);
sig_t _pthread_signal(int s, sig_t h);
void _pthread_queue_signal(int s);
void _pthread_process_signal();
int _pthread_has_signal(int s);
int _pthread_stop(pthread_t id);
int _pthread_suspend(pthread_t id);
int _pthread_resume(pthread_t id);
void _pthread_mutex_free();

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_attr_setinitialstate(pthread_attr_t *attr, int initial_state);

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *args);

    
int pthread_join(pthread_t thread, void **value_ptr);

int pthread_key_create(pthread_key_t *k, void (*destructor)(void*));
int pthread_setspecific(pthread_key_t k, const void *value);
void *pthread_getspecific(pthread_key_t k);

int pthread_mutex_init(pthread_mutex_t *mut, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mut);
int pthread_mutex_unlock(pthread_mutex_t *mut);
int pthread_mutex_trylock(pthread_mutex_t *mut);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
    
pthread_t pthread_self(void);

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);

void pthread_cleanup_push(void (*routine)(void *), void *arg);

// Mutex types
#define PTHREAD_MUTEX_NORMAL        1
#define PTHREAD_MUTEX_ERRORCHECK    2
#define PTHREAD_MUTEX_RECURSIVE     3
#define PTHREAD_MUTEX_DEFAULT       4

// Initializers
#define PTHREAD_MUTEX_INITIALIZER  0
#define PTHREAD_ONCE_INIT          {NULL}
#define PTHREAD_COND_INITIALIZER   {NULL}

sig_t _pthread_signal(int s, sig_t h);
        
#endif	/* PTHREAD_H */


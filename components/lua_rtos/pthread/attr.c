/*
 * Lua RTOS, pthread implementation over FreeRTOS
 *
 * Copyright (C) 2015 - 2017
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

#include "luartos.h"

#include "freertos/FreeRTOS.h"

#include <errno.h>
#include <pthread/pthread.h>

int pthread_attr_init(pthread_attr_t *attr) {
    attr->stack_size = CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE;
    attr->initial_state = PTHREAD_INITIAL_STATE_RUN;
    attr->sched_priority = CONFIG_LUA_RTOS_LUA_TASK_PRIORITY;
    attr->cpuset = tskNO_AFFINITY;
    
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr) {
    return 0;
    
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
    if (stacksize < PTHREAD_STACK_MIN) {
        errno = EINVAL;
        return errno;
    }
    
    attr->stack_size = stacksize;
    
    return 0;
}

int pthread_attr_setinitialstate(pthread_attr_t *attr, int initial_state) {
    if ((initial_state != PTHREAD_INITIAL_STATE_RUN) && (initial_state != PTHREAD_INITIAL_STATE_SUSPEND)) {
        errno = EINVAL;
        return errno;
    }
    
    attr->initial_state = initial_state;
    
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize) {
    *stacksize = attr->stack_size;
    
    return 0;
}

int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param) {
    if ((param->sched_priority > configMAX_PRIORITIES - 1) || (param->sched_priority < 1)) {
        errno = EINVAL;
        return errno;
    }

    attr->sched_priority = param->sched_priority;

    return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param) {
	param->sched_priority = attr->sched_priority;

	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
    return 0;
}

int pthread_setcancelstate(int state, int *oldstate) {
    return 0;
}

int pthread_attr_setaffinity_np(pthread_attr_t *attr, size_t cpusetsize, const cpu_set_t *cpuset) {
	if ((*cpuset < 0) || (*cpuset > portNUM_PROCESSORS)) {
		errno = EINVAL;
		return errno;
	}

	attr->cpuset = *cpuset;

	return 0;
}

int pthread_attr_getaffinity_np(const pthread_attr_t *attr, size_t cpusetsize, cpu_set_t *cpuset) {
	if ((*cpuset < 0) || (*cpuset > portNUM_PROCESSORS)) {
		errno = EINVAL;
		return errno;
	}

	*cpuset = attr->cpuset;

	return 0;
}

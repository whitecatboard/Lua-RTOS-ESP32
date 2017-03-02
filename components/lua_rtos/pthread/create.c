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

#include <errno.h>
#include <pthread/pthread.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *args) {
    
	int priority;      // Priority
    int stacksize;     // Stack size
    int initial_state; // Initial state
    int cpu;           // CPU affinity

    int res;

    // Get some arguments need for the thread creation
    if (attr) {
        stacksize = attr->stack_size;
        if (stacksize < PTHREAD_STACK_MIN) {
            errno = EINVAL;
            return EINVAL;
        }
        priority = attr->sched_priority;
        initial_state = attr->initial_state;
        cpu = attr->cpuset;
    } else {
        stacksize = CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE;
        initial_state = PTHREAD_INITIAL_STATE_RUN;
        priority = CONFIG_LUA_RTOS_LUA_TASK_PRIORITY;

        if (portNUM_PROCESSORS > 0) {
        	cpu = 0;
        } else {
        	cpu = tskNO_AFFINITY;
        }
    }
     
    // Create a new pthread
    res = _pthread_create(thread, priority, stacksize, cpu, initial_state, start_routine, args);
    if (res) {
        errno = res;
        return res;
    }
       
    return 0;
}

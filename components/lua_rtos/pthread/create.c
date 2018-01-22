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
#include "_pthread.h"

#include <errno.h>
#include <pthread.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *args) {
    
	int priority;      // Priority
    int stacksize;     // Stack size
    int initial_state; // Initial state

    cpu_set_t cpu_set = CPU_INITIALIZER;

    int res;

    // Get some arguments need for the thread creation
    if (attr) {
        stacksize = attr->stacksize;
        if (stacksize < PTHREAD_STACK_MIN) {
            errno = EINVAL;
            return EINVAL;
        }
        priority = attr->schedparam.sched_priority;
        cpu_set = attr->schedparam.affinityset;
        initial_state = attr->schedparam.initial_state;
    } else {
        stacksize = CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE;
        initial_state = PTHREAD_INITIAL_STATE_RUN;
        priority = CONFIG_LUA_RTOS_LUA_TASK_PRIORITY;
    }

    // CPU affinity
    int cpu = 0;

    if (cpu_set != CPU_INITIALIZER) {
    	if (CPU_ISSET(0, &cpu_set)) {
    		cpu = 0;
    	} else if (CPU_ISSET(1, &cpu_set)) {
    		cpu = 1;
    	}
    } else {
    	cpu = tskNO_AFFINITY;
    }

    // Create a new pthread
    res = _pthread_create(thread, priority, stacksize, cpu, initial_state, start_routine, args);
    if (res) {
        errno = res;
        return res;
    }
       
    return 0;
}

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

#include "_pthread.h"

int pthread_attr_init(pthread_attr_t *attr) {
	if (!attr) {
		return EINVAL;
	}

	attr->is_initialized = 1;

	attr->stacksize = (CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE >= PTHREAD_STACK_MIN?CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE:PTHREAD_STACK_MIN);

    attr->schedparam.initial_state = PTHREAD_INITIAL_STATE_RUN;
    attr->schedparam.sched_priority = CONFIG_LUA_RTOS_LUA_TASK_PRIORITY;
	attr->schedparam.affinityset = CPU_INITIALIZER; // No affinity
	attr->init_func = NULL;
	attr->detachstate = PTHREAD_CREATE_JOINABLE;

    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr) {
	if (!attr) {
		return EINVAL;
	}

	if (!attr->is_initialized) {
		return EINVAL;
	}

	return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
	if (!attr) {
		return EINVAL;
	}

    if (stacksize < PTHREAD_STACK_MIN) {
        return EINVAL;
    }

	if (!attr->is_initialized) {
		return EINVAL;
	}

    attr->stacksize = stacksize;
    
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize) {
	if (!attr || !stacksize) {
		return EINVAL;
	}

	if (!attr->is_initialized) {
		return EINVAL;
	}

    *stacksize = attr->stacksize;
    
    return 0;
}

int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param) {
	if (!attr || !param) {
		return EINVAL;
	}

	if (!attr->is_initialized) {
		return EINVAL;
	}

	if ((param->sched_priority > configMAX_PRIORITIES - 1) || (param->sched_priority < 1)) {
        return EINVAL;
    }

    attr->schedparam.sched_priority = param->sched_priority;

    return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param) {
	if (!attr || !param) {
		return EINVAL;
	}

	if (!attr->is_initialized) {
		return EINVAL;
	}

	param->sched_priority = attr->schedparam.sched_priority;

	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
	if (!attr) {
		return EINVAL;
	}

	if (!attr->is_initialized) {
		return EINVAL;
	}

	if ((detachstate != PTHREAD_CREATE_DETACHED) && (detachstate != PTHREAD_CREATE_JOINABLE)) {
		return EINVAL;
	}

	attr->detachstate = detachstate;

    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {
	if (!attr || !detachstate) {
		return EINVAL;
	}

	if (!attr->is_initialized) {
		return EINVAL;
	}

	*detachstate = attr->detachstate;

	return 0;
}

int pthread_setcancelstate(int state, int *oldstate) {
    return 0;
}

int pthread_attr_setaffinity_np(pthread_attr_t *attr, size_t cpusetsize, const cpu_set_t *cpuset) {
	if (!attr || !cpuset) {
		return EINVAL;
	}

	if (!attr->is_initialized) {
		return EINVAL;
	}

	if ((*cpuset < 0) || (*cpuset > portNUM_PROCESSORS)) {
		return EINVAL;
	}

	attr->schedparam.affinityset = *cpuset;

	return 0;
}

int pthread_attr_getaffinity_np(const pthread_attr_t *attr, size_t cpusetsize, cpu_set_t *cpuset) {
	if (!attr || !cpuset) {
		return EINVAL;
	}

	if (!attr->is_initialized) {
		return EINVAL;
	}

	if ((*cpuset < 0) || (*cpuset > portNUM_PROCESSORS)) {
		return EINVAL;
	}

	*cpuset = attr->schedparam.affinityset;

	return 0;
}

int pthread_attr_setinitialstate_np(pthread_attr_t *attr, int initial_state) {
	if (!attr) {
		return EINVAL;
	}

	if (!attr->is_initialized) {
		return EINVAL;
	}

    if ((initial_state != PTHREAD_INITIAL_STATE_RUN) && (initial_state != PTHREAD_INITIAL_STATE_SUSPEND)) {
        return EINVAL;
    }

    attr->schedparam.initial_state = initial_state;

    return 0;
}

int pthread_attr_setinitfunc_np(pthread_attr_t *attr, void (*init_routine)(void *)) {
	if (!attr || !init_routine) {
		return EINVAL;
	}

	if (!attr->is_initialized) {
		return EINVAL;
	}

    attr->init_func = init_routine;

    return 0;
}

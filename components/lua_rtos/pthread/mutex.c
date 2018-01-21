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

#include "esp_attr.h"

#include <errno.h>
#include <stdlib.h>

static int _check_attr(const pthread_mutexattr_t *attr) {
    int type = attr->type;
        
    if ((type < PTHREAD_MUTEX_NORMAL) || (type > PTHREAD_MUTEX_DEFAULT)) {
        errno = EINVAL;
        return EINVAL;
    }
   
   return 0;
}

int pthread_mutex_init(pthread_mutex_t *mut, const pthread_mutexattr_t *attr) {
    struct pthread_mutex *mutex;
    int res;

    if (!mut) {
        return EINVAL;
    }

    // Check attr
    if (attr) {
        res = _check_attr(attr);
        if (res) {
            errno = res;
            return res;
        }
    }

    // Test if it's init yet
    if (*mut != PTHREAD_MUTEX_INITIALIZER) {
        errno = EBUSY;
        return EBUSY;
    }

    // Create mutex structure
    mutex = (struct pthread_mutex *)malloc(sizeof(struct pthread_mutex));
    if (!mutex) {
        errno = EINVAL;
        return EINVAL;
    }

    if (attr) {
    	mutex->type = attr->type;
    } else {
    	mutex->type = PTHREAD_MUTEX_NORMAL;
    }
    // Create semaphore
    if (mutex->type == PTHREAD_MUTEX_RECURSIVE) {
        mutex->sem = xSemaphoreCreateRecursiveMutex();    
    } else {
        mutex->sem = xSemaphoreCreateMutex();
    }
    if(!mutex->sem){
        *mut = PTHREAD_MUTEX_INITIALIZER;
        free(mutex->sem);
        free(mutex);
        errno = ENOMEM;
        return ENOMEM;
    }

    mutex->owner = pthread_self();

    *mut = (unsigned int )mutex;

    return 0;    
}

int IRAM_ATTR pthread_mutex_lock(pthread_mutex_t *mut) {
    struct pthread_mutex *mutex;
    int res;

    if (!mut) {
        return EINVAL;
    }

    if ((intptr_t) *mut == PTHREAD_MUTEX_INITIALIZER) {
    	if ((res = pthread_mutex_init(mut, NULL))) {
    		return res;
    	}
    }

    mutex = (struct pthread_mutex *)(*mut);

    // Lock
    if (mutex->type == PTHREAD_MUTEX_RECURSIVE) {
        if (xSemaphoreTakeRecursive(mutex->sem, PTHREAD_MTX_LOCK_TIMEOUT) != pdPASS) {
            PTHREAD_MTX_DEBUG_LOCK();
            errno = EINVAL;
            return EINVAL;
        }
    } else {
        if (xSemaphoreTake(mutex->sem, PTHREAD_MTX_LOCK_TIMEOUT) != pdPASS) {
            PTHREAD_MTX_DEBUG_LOCK();
            errno = EINVAL;
            return EINVAL;
        }        
    }
    
    return 0;
}

int IRAM_ATTR pthread_mutex_unlock(pthread_mutex_t *mut) {
	struct pthread_mutex *mutex = ( struct pthread_mutex *)(*mut);

    if (!mut) {
        return EINVAL;
    }

    // Unlock
    if (mutex->type == PTHREAD_MUTEX_RECURSIVE) {
        xSemaphoreGiveRecursive(mutex->sem);
    } else {
        xSemaphoreGive(mutex->sem);
    }

    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mut) {
	struct pthread_mutex *mutex;
	int res;

    if (!mut) {
        return EINVAL;
    }

    if ((intptr_t) *mut == PTHREAD_MUTEX_INITIALIZER) {
    	if ((res = pthread_mutex_init(mut, NULL))) {
    		return res;
    	}
    }

    mutex = ( struct pthread_mutex *)(*mut);

    // Try lock
    if (mutex->type == PTHREAD_MUTEX_RECURSIVE) {
        if (xSemaphoreTakeRecursive(mutex->sem,0 ) != pdTRUE) {
            errno = EBUSY;
            return EBUSY;
        }
    } else {
        if (xSemaphoreTake(mutex->sem,0 ) != pdTRUE) {
            errno = EBUSY;
            return EBUSY;
        }
    }
    
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mut) {
    if (!mut) {
        return EINVAL;
    }

    struct pthread_mutex *mutex = ( struct pthread_mutex *)(*mut);

    if (mutex->type == PTHREAD_MUTEX_RECURSIVE) {
        xSemaphoreGiveRecursive(mutex->sem);
    } else {
        xSemaphoreGive(mutex->sem);
    }
    
    vSemaphoreDelete(mutex->sem);

    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
    pthread_mutexattr_t temp_attr;
    int res;

    // Check attr
    if (!attr) {
        errno = EINVAL;
        return EINVAL;
    }

    temp_attr.type = type;
    
    res = _check_attr(&temp_attr);
    if (res) {
        errno = res;
        return res;
    }

    attr->type = type;
    
    return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr) {
    if (!attr) {
        return EINVAL;
    }

    attr->type = PTHREAD_MUTEX_NORMAL;
    attr->is_initialized = 1;

    return 0;
}

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

#include <errno.h>
#include <sys/mutex.h>

extern struct mtx cond_mtx;

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
	// Atrr values in this implementation are not allowed
	if (attr) {
		mtx_unlock(&cond_mtx);
		return ENOMEM;
	}

	mtx_lock(&cond_mtx);

	struct pthread_cond *scond;

	if (*cond != PTHREAD_COND_INITIALIZER) {
		scond = calloc(1, sizeof(struct pthread_cond));
		if (!scond) {
        	mtx_unlock(&cond_mtx);
			return ENOMEM;
		}
		scond->referenced = 0;
	} else {
		scond = (struct pthread_cond *)cond;
	}

	if (scond->referenced > 0) {
		mtx_unlock(&cond_mtx);
		if (*cond != PTHREAD_COND_INITIALIZER) free(scond);
		return EBUSY;
	}

    if (!scond->mutex.lock) {
        mtx_init(&scond->mutex, NULL, NULL, 0);
        if (!scond->mutex.lock) {
        	mtx_unlock(&cond_mtx);
        	if (*cond != PTHREAD_COND_INITIALIZER) free(scond);
        	return ENOMEM;
        }
    } else {
    	mtx_unlock(&cond_mtx);
    	if (*cond != PTHREAD_COND_INITIALIZER) free(scond);
    	return EBUSY;
    }
    
    scond->referenced = 0;

    mtx_unlock(&cond_mtx);

	return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
	mtx_lock(&cond_mtx);

	if (*cond == PTHREAD_COND_INITIALIZER) {
    	mtx_unlock(&cond_mtx);
    	return EINVAL;
	}

	struct pthread_cond *scond = (struct pthread_cond *)cond;

	if (scond->referenced > 0) {
		mtx_unlock(&cond_mtx);
		return EBUSY;
	}

    if (scond->mutex.lock) {
        mtx_destroy(&scond->mutex);
    } else {
    	mtx_unlock(&cond_mtx);
    	return EINVAL;
    }

    mtx_unlock(&cond_mtx);
	
	return 0; 
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {

	if (*cond == PTHREAD_COND_INITIALIZER) {
    	mtx_unlock(&cond_mtx);
    	return EINVAL;
	}

	struct pthread_cond *scond = (struct pthread_cond *)cond;

	if (!scond->mutex.lock) {
		return EINVAL;
	}

	// At this point cond is protected by mutex, so is not necessary to use
	// cond_mtx
	scond->referenced++;

    // Wait for condition
    mtx_lock(&scond->mutex);

    // If current thread is the first that calls to pthread_cond_wait, lock
    // again for block thread
    if (scond->referenced == 1) {
        mtx_lock(&scond->mutex);

    }

    pthread_mutex_unlock(mutex);

    return 0;
}
  
int pthread_cond_timedwait(pthread_cond_t *cond, 
    pthread_mutex_t *mutex, const struct timespec *abstime) { 
    
	if (*cond == PTHREAD_COND_INITIALIZER) {
    	mtx_unlock(&cond_mtx);
    	return EINVAL;
	}

	struct pthread_cond *scond = (struct pthread_cond *)cond;

	struct pthread_mutex *smutex = (struct pthread_mutex *)mutex;

	if (!smutex->sem) {
		return EINVAL;
	}

	// At this point cond is protected by mutex, so is not necessary to use
	// cond_mtx
	scond->referenced++;

    // If current thread is the first that calls to pthread_cond_wait, lock
    // again for block thread
    if (scond->referenced == 1) {
        mtx_lock(&scond->mutex);
    }

    // Wait for condition
    if (xSemaphoreTake(scond->mutex.lock, (1000 * abstime->tv_sec) / portTICK_PERIOD_MS ) != pdTRUE) {
        return ETIMEDOUT;
    }
    
    pthread_mutex_unlock(mutex);
    
    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond) {
	if (*cond == PTHREAD_COND_INITIALIZER) {
    	mtx_unlock(&cond_mtx);
    	return EINVAL;
	}

	struct pthread_cond *scond = (struct pthread_cond *)cond;

	if (!scond->mutex.lock) {
		return EINVAL;
	}

	// At this point cond is protected by mutex, prior to calling pthread_cond_wait or
	// pthread_cond_timedwait. pthread_cond_init / pthread_cond_destroy fails due to
	// referenced > 0, so is not necessary to use cond_mtx
	scond->referenced--;

	// Release cond
	mtx_unlock(&scond->mutex);

    return 0;
}

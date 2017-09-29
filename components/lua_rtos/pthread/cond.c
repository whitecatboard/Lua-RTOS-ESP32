/*
 * Lua RTOS, pthread implementation over FreeRTOS
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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
	} else {
		scond = (struct pthread_cond *)cond;
	}

	if (scond->referenced > 0) {
		mtx_unlock(&cond_mtx);
		return EBUSY;
	}

    if (!scond->mutex.sem) {
        mtx_init(&scond->mutex, NULL, NULL, 0);
        if (!scond->mutex.sem) {
        	mtx_unlock(&cond_mtx);
        	return ENOMEM;
        }
    } else {
    	mtx_unlock(&cond_mtx);
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

    if (scond->mutex.sem) {
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

	if (!scond->mutex.sem) {
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
    if (xSemaphoreTake(scond->mutex.sem, (1000 * abstime->tv_sec) / portTICK_PERIOD_MS ) != pdTRUE) {
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

	if (!scond->mutex.sem) {
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

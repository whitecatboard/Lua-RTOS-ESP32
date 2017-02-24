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

#include <errno.h>
#include <sys/mutex.h>
#include <pthread/pthread.h>

extern struct mtx cond_mtx;

#if MTX_USE_EVENTS
extern eventg_t eventg[MTX_EVENT_GROUPS];
#endif

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
	// Atrr values in this implementation are not allowed
	if (attr) {
		return EINVAL;
	}

	mtx_lock(&cond_mtx);

	if (cond->referenced > 0) {
		mtx_unlock(&cond_mtx);
		return EBUSY;
	}

	#if !MTX_USE_EVENTS
    if (!cond->mutex.sem) {
        mtx_init(&cond->mutex, NULL, NULL, 0);
        if (!cond->mutex.sem) {
        	mtx_unlock(&cond_mtx);
        	return ENOMEM;
        }
    } else {
    	mtx_unlock(&cond_mtx);
    	return EBUSY;
    }
	#else
    if (cond->mutex.mtxid < 0) {
        mtx_init(&cond->mutex, NULL, NULL, 0);
        if (cond->mutex.mtxid < 0) {
        	mtx_unlock(&cond_mtx);
        	return ENOMEM;
        }
    } else {
    	mtx_unlock(&cond_mtx);
    	return EBUSY;
    }
	#endif
    
    cond->referenced = 0;

    mtx_unlock(&cond_mtx);

	return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
	mtx_lock(&cond_mtx);

	if (cond->referenced > 0) {
		mtx_unlock(&cond_mtx);
		return EBUSY;
	}

	#if !MTX_USE_EVENTS
    if (cond->mutex.sem) {
        mtx_destroy(&cond->mutex);
    } else {
    	mtx_unlock(&cond_mtx);
    	return EINVAL;
    }
	#else
    if (cond->mutex.mtxid >= 0) {
        mtx_destroy(&cond->mutex);
    } else {
    	mtx_unlock(&cond_mtx);
    	return EINVAL;
    }
	#endif

    mtx_unlock(&cond_mtx);
	
	return 0; 
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
	#if !MTX_USE_EVENTS
	if (!cond->mutex.sem) {
		return EINVAL;
	}
	#else
	if (cond->mutex.mtxid < 0) {
		return EINVAL;
	}
	#endif

	// At this point cond is protected by mutex, so is not necessary to use
	// cond_mtx
	cond->referenced++;

    // Wait for condition
    mtx_lock(&cond->mutex);

    // If current thread is the first that calls to pthread_cond_wait, lock
    // again for block thread
    if (cond->referenced == 1) {
        mtx_lock(&cond->mutex);

    }

    pthread_mutex_unlock(mutex);

    return 0;
}
  
int pthread_cond_timedwait(pthread_cond_t *cond, 
    pthread_mutex_t *mutex, const struct timespec *abstime) { 
    
	#if !MTX_USE_EVENTS
	if (!cond->mutex.sem) {
		return EINVAL;
	}
	#else
	if (cond->mutex.mtxid < 0) {
		return EINVAL;
	}
	#endif

	// At this point cond is protected by mutex, so is not necessary to use
	// cond_mtx
	cond->referenced++;

    // If current thread is the first that calls to pthread_cond_wait, lock
    // again for block thread
    if (cond->referenced == 1) {
        mtx_lock(&cond->mutex);
    }

    // Wait for condition
	#if !MTX_USE_EVENTS
    if (xSemaphoreTake(cond->mutex.sem, (1000 * abstime->tv_sec) / portTICK_PERIOD_MS ) != pdTRUE) {
        return ETIMEDOUT;
    }
	#else
  	EventBits_t uxBits = xEventGroupWaitBits(MTX_EVENTG(cond->mutex.mtxid).eg, MTX_EVENTG_BIT(cond->mutex.mtxid), pdTRUE, pdTRUE, (1000 * abstime->tv_sec) / portTICK_PERIOD_MS );
  	if (!(uxBits & MTX_EVENTG_BIT(cond->mutex.mtxid))) {
        return ETIMEDOUT;
  	}
	#endif
    
    pthread_mutex_unlock(mutex);
    
    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond) {
	#if !MTX_USE_EVENTS
	if (!cond->mutex.sem) {
		return EINVAL;
	}
	#else
	if (cond->mutex.mtxid < 0) {
		return EINVAL;
	}
	#endif

	// At this point cond is protected by mutex, prior to calling pthread_cond_wait or
	// pthread_cond_timedwait. pthread_cond_init / pthread_cond_destroy fails due to
	// cond->referenced > 0, so is not necessary to use cond_mtx
	cond->referenced--;

	// Release cond
	mtx_unlock(&cond->mutex);

    return 0;
}

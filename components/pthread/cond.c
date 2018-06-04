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

#include <sys/mutex.h>
#include <sys/time.h>

#if configUSE_16_BIT_TICKS
#define BITS_PER_EVENT_GROUP 8
#else
#define BITS_PER_EVENT_GROUP 24
#endif

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
	// Avoid to reinitialize the object referenced by cond, a previously
	// initialized, but not yet destroyed, condition variable
	if (*cond != PTHREAD_COND_INITIALIZER) {
		return EBUSY;
	}

	// Initialize the internal cond structure
	struct pthread_cond *scond = calloc(1, sizeof(struct pthread_cond));
	if (!scond) {
		return ENOMEM;
	}

	// Init the cond mutex
	mtx_init(&scond->mutex, NULL, NULL, 0);
	if (!scond->mutex.lock) {
		free(scond);
		return ENOMEM;
	}

	// Create the event group
	scond->ev = xEventGroupCreate();
	if (scond->ev == NULL) {
		mtx_destroy(&scond->mutex);
		free(scond);
		return ENOMEM;
	}

	// Return the cond reference
	*cond = (pthread_cond_t)scond;

	return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
	if (*cond == PTHREAD_COND_INITIALIZER) {
		return EINVAL;
	}

	struct pthread_cond *scond = (struct pthread_cond *) *cond;

	mtx_lock(&scond->mutex);

	if (scond->referenced > 0) {
		mtx_unlock(&scond->mutex);

		return EBUSY;
	}

	mtx_destroy(&scond->mutex);

	return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
	return pthread_cond_timedwait(cond, mutex, NULL);
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
		const struct timespec *abstime) {

	if (*cond == PTHREAD_COND_INITIALIZER) {
		return EINVAL;
	}

	struct pthread_cond *scond = (struct pthread_cond *)*cond;

	// Find an unused bit in the event group to sync the condition
	mtx_lock(&scond->mutex);

	uint8_t bit;
	for(bit=0;bit < BITS_PER_EVENT_GROUP;bit++) {
		if (!((1 << bit) & scond->referenced)) {
			scond->referenced |= (1 << bit);

			break;
		}
	}

	mtx_unlock(&scond->mutex);

	if (bit >= BITS_PER_EVENT_GROUP) {
		// All bits are used in the event group. This is a rare condition, because
		// in Lua RTOS an event group has 24 bits, which means that 24 conditions
		// (or 24 threads) can be handled for each condition variable.
		//
		// Although is not part of the pthread specification we indicate this
		// condition returning the EINVAL error code.
		return EINVAL;
	}

	// Get the timeout in ticks
	TickType_t ticks;

	if (!abstime) {
		ticks = portMAX_DELAY;
	} else {
		struct timeval now, future, diff;

		gettimeofday(&now, NULL);

		future.tv_sec = abstime->tv_sec;
		future.tv_usec = abstime->tv_nsec / 1000;

		if (timercmp(&future, &now, <)) {
			return ETIMEDOUT;
		} else {
			timersub(&future, &now, &diff);
			ticks = ((diff.tv_sec * 1000) + (diff.tv_usec / 1000)) / portTICK_PERIOD_MS;
		}
	}

	pthread_mutex_unlock(mutex);
	EventBits_t uxBits = xEventGroupWaitBits(scond->ev, (1 << bit), pdTRUE, pdTRUE, ticks);
	if (!(uxBits & (1 << bit))) {
	    pthread_mutex_lock(mutex);
	    scond->referenced &= ~(1 << bit);
	    pthread_mutex_unlock(mutex);

		return ETIMEDOUT;
	}
	pthread_mutex_lock(mutex);

	return 0;
}

int pthread_cond_signal(pthread_cond_t *cond) {
	if (*cond == PTHREAD_COND_INITIALIZER) {
		return EINVAL;
	}

	struct pthread_cond *scond = (struct pthread_cond *)*cond;

	mtx_lock(&scond->mutex);

	uint8_t bit;
	for(bit=0;bit < BITS_PER_EVENT_GROUP;bit++) {
		if ((1 << bit) & scond->referenced) {
			xEventGroupSetBits(scond->ev, (1 << bit));

			scond->referenced &= ~(1 << bit);
		}
	}

	mtx_unlock(&scond->mutex);

	return 0;
}

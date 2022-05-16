/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS nanosleep implementation.
 * The time granularity is 1 millisecond.
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <pthread.h>
#include <errno.h>
#include <time.h>

#include <sys/time.h>
#include <sys/signal.h>

int nanosleep(const struct timespec *req, struct timespec *rem) {
	struct timeval start, end;
	int res = 0;

	if ((req->tv_nsec < 0) || (req->tv_nsec > 999999999)) {
		errno = EINVAL;

		return -1;
	}

	// Get time in msecs
	uint32_t msecs;

	msecs  = req->tv_sec * 1000;
	msecs += (req->tv_nsec + 999999) / 1000000;

	if (rem != NULL) {
		gettimeofday(&start, NULL);
	}

	if ((res = _pthread_sleep(msecs)) != 0) {
		if (rem != NULL) {
			rem->tv_sec = 0;
			rem->tv_nsec = 0;

			gettimeofday(&end, NULL);

			uint32_t elapsed = (end.tv_sec - start.tv_sec) * 1000
							 + ((end.tv_usec - start.tv_usec) / 1000000);

			if (elapsed < msecs) {
				if (elapsed > 1000) {
					rem->tv_sec = elapsed / 1000;
					elapsed -= rem->tv_sec * 1000;
					rem->tv_nsec = elapsed * 1000000;
				}
			}
		}
		errno = EINTR;
	}

	return res;
}

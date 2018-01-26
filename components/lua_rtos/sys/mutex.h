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
 * Lua RTOS mutex api implementation over FreeRTOS
 *
 */

#ifndef _MUTEX_H
#define	_MUTEX_H

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#if !CONFIG_LUA_RTOS_USE_EVENT_GROUP_IN_MTX
struct mtx {
    SemaphoreHandle_t lock;
};
#else
#define MTX_MAX 44
#include "freertos/event_groups.h"

// Get the mutex id, witch is calculated an event group identifier and
// bit position within the event group
//
// bits 31 to 24 contains the event group identifier, and bits 23 to 0
// contains the bit position within the event groupp
#define MTX_ID(eventgid, bit) ((uint32_t)(eventgid << 24) | (uint32_t)((1 << bit) & 0x00ffffff))

// Get the event group identifier from a mutex id
#define MTX_EVENTG_ID(mtxid) ((mtxid >> 24) & 0x000000ff)

// Get the event group bit from a mutex id
#define MTX_EVENTG_BIT(mtxid) (mtxid &0x00ffffff)

// Get the vent group from the mutext control structure that corresponds to
// the mutex id
#define MTX_EVENTG(mtxid) eventg[MTX_EVENTG_ID(mtxid)]

// Get the number of bits available in each event group
#if configUSE_16_BIT_TICKS
#define BITS_PER_EVENT_GROUP 8
#else
#define BITS_PER_EVENT_GROUP 24
#endif

// Get the number of event groups needed for manage MTX_MAX mutexes
#define MTX_EVENT_GROUPS (MTX_MAX / BITS_PER_EVENT_GROUP) + 1

typedef struct {
	uint32_t used;
	EventGroupHandle_t eg;
} eventg_t;

struct mtx {
    uint32_t lock; // mutex id
};
#endif

void _mtx_init();
void mtx_init(struct mtx *mutex, const char *name, const char *type, int opts);
void mtx_lock(struct mtx *mutex);
int  mtx_trylock(struct	mtx *mutex);
void mtx_unlock(struct mtx *mutex);
void mtx_destroy(struct	mtx *mutex);

#endif	/* _MUTEX_H */


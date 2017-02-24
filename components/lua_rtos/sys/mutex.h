/*
 * Lua RTOS, mutex api implementation over FreeRTOS
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

#ifndef MUTEX_H_H
#define	MUTEX_H_H

#include "freertos/FreeRTOS.h"

#if !MTX_USE_EVENTS
#include "freertos/semphr.h"

#define MUTEX_INITIALIZER {.sem = 0}

struct mtx {
    SemaphoreHandle_t sem;
};

#else

#define MTX_MAX 100
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
    uint32_t mtxid; // mutex id
};
#endif


void mtx_init(struct mtx *mutex, const char *name, const char *type, int opts);
void mtx_lock(struct mtx *mutex);
int  mtx_trylock(struct	mtx *mutex);
void mtx_unlock(struct mtx *mutex);
void mtx_destroy(struct	mtx *mutex);

#endif	/* MUTEX_H_H */


/*
 * Lua RTOS, Lua event module
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

/*
 * This module implements basic event driven mechanism for sync threads.
 */

#ifndef LEVENT_H
#define	LEVENT_H

#include "lua.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <sys/mutex.h>
#include <sys/list.h>

#include <pthread/pthread.h>

typedef struct {
	pthread_t thread;   // Thread id
	uint8_t is_waiting; // If 1 the caller is waiting for the termination of all subscribers
	xQueueHandle q;     // Queue for sync listener thread with this event
} listener_data_t;

typedef struct {
	struct mtx mtx;        // Mutex for protect the listeners list and the queue
	struct list listeners; // List of listeners for this event
	uint8_t pending;       // Number of listeners that are processing the event
	uint8_t waiting_for;   // Number of listener that caller must be wait for it's termination
	xQueueHandle q;        // This is used by the listener for inform the caller that event is processed
} event_userdata_t;

#endif	/* LEVENT_H */

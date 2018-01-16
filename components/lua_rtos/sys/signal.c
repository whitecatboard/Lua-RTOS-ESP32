/*
 * Lua RTOS, minimal signal implementation
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

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <pthread.h>
#include <signal.h>
#include <sys/_signal.h>

static xQueueHandle queue = NULL;

static void signal_task(void *args) {
	signal_data_t data;

	for(;;) {
		xQueueReceive(queue, &data, portMAX_DELAY);
		_pthread_exec_signal(data.dest, data.s);
	}
}

void _signal_init() {
	// Create queue to receive signals
	queue = xQueueCreate(10, sizeof(signal_data_t));
	assert(queue != NULL);

	// Create signal task
	BaseType_t ret = xTaskCreatePinnedToCore(signal_task, "signal", configMINIMAL_STACK_SIZE, NULL, 21, NULL, 0);
	assert(ret == pdPASS);
}

void _signal_queue(int dest, int s) {
	signal_data_t data;

	data.dest = dest;
	data.s = s;

	xQueueSend(queue, &data, 0);
}

sig_t signal(int s, sig_t h) {
    return _pthread_signal(s, h);
}

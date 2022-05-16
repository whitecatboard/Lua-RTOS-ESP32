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
 * Lua RTOS minimal signal implementation
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <pthread.h>
#include <signal.h>
#include <sys/list.h>
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
	BaseType_t ret = xTaskCreatePinnedToCore(signal_task, "signal", configMINIMAL_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_TASK_PRIORITY + 1, NULL, 0);
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

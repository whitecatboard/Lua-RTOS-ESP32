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
 * Lua RTOS main start
 *
 */

#include <sys/features.h>

#include "luartos.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sys/debug.h>
#include <sys/panic.h>
#include <sys/mount.h>

#include <drivers/gpio.h>

#include <pthread.h>

void luaos_main();
void _sys_init();

pthread_t lua_thread;

void *lua_start(void *arg) {
	for(;;) {
		luaos_main();
	}

	return NULL;
}

void app_main() {
	_sys_init();

	#if (CONFIG_LUA_RTOS_LED_ACT >= 0)
	driver_lock(SYSTEM_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_LED_ACT, DRIVER_ALL_FLAGS, "Activity LED");

	// Init leds
	gpio_pin_output(CONFIG_LUA_RTOS_LED_ACT);
	gpio_pin_clr(CONFIG_LUA_RTOS_LED_ACT);
	#endif

	// Create and run a pthread for the Lua interpreter
	pthread_attr_t attr;
	struct sched_param sched;
	int res;

	debug_free_mem_begin(lua_main_thread);

	// Init thread attributes
	pthread_attr_init(&attr);

	// Set stack size
	pthread_attr_setstacksize(&attr, CONFIG_LUA_RTOS_LUA_STACK_SIZE);

	// Set priority
	sched.sched_priority = CONFIG_LUA_RTOS_LUA_TASK_PRIORITY;
	pthread_attr_setschedparam(&attr, &sched);

	// Set CPU
	cpu_set_t cpu_set = CPU_INITIALIZER;

	CPU_SET(CONFIG_LUA_RTOS_LUA_TASK_CPU, &cpu_set);

	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	// Create thread
	res = pthread_create(&lua_thread, &attr, lua_start, NULL);
	if (res) {
		panic("Cannot start lua");
	}

	pthread_setname_np(lua_thread, "lua_main");
	debug_free_mem_end(lua_main_thread, NULL);
}

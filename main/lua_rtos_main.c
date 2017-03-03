/*
 * Lua RTOS, main start program
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

#include "luartos.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sys/debug.h>
#include <sys/panic.h>
#include <sys/mount.h>

#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <drivers/pwm.h>

#include <pthread/pthread.h>

void luaos_main();
void _sys_init();

void *lua_start(void *arg) {	
	for(;;) {
		luaos_main();
    }

    return NULL;
}

int linenoise(char *buf, const char *prompt);

void app_main() {
    nvs_flash_init();
	
	_sys_init();

	#if USE_LED_ACT
	// Init leds
	gpio_pin_output(LED_ACT);
	gpio_pin_clr(LED_ACT);
	#endif

	// Create and run a pthread for the Lua interpreter
	pthread_attr_t attr;
	struct sched_param sched;
	pthread_t thread;
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
    cpu_set_t cpu_set = CONFIG_LUA_RTOS_LUA_TASK_CPU;
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);

    // Create thread
    res = pthread_create(&thread, &attr, lua_start, NULL);
    if (res) {
		panic("Cannot start lua");
	}
	
	debug_free_mem_end(lua_main_thread, NULL);	
}

/*
 * Lua RTOS, timer driver
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

#include "sdkconfig.h"
#include "timer.h"
#include "esp_attr.h"
#include "driver/timer.h"
#include "soc/timer_group_struct.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <math.h>
#include <string.h>

#include <sys/mutex.h>

#include <drivers/cpu.h>
#include <drivers/timer.h>

// Driver message errors
DRIVER_REGISTER_ERROR(TIMER, timer, InvalidUnit, "invalid unit", TIMER_ERR_INVALID_UNIT);
DRIVER_REGISTER_ERROR(TIMER, timer, NotEnoughtMemory, "not enough memory", TIMER_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_ERROR(TIMER, timer, NoMoreTimers, "no more timers available", TIMER_ERR_NO_MORE_TIMERS);
DRIVER_REGISTER_ERROR(TIMER, timer, InvalidPeriod, "invalid period", TIMER_ERR_INVALID_PERIOD);
DRIVER_REGISTER_ERROR(TIMER, timer, NotSetup, "is not setup", TIMER_ERR_IS_NOT_SETUP);

// Timer array with needed information about timers
static tmr_t tmr[CPU_LAST_TIMER + 1];

// Alarm queue
static xQueueHandle queue = NULL;

/*
 * Helper functions
 */

static void tmr_init() {
	memset(tmr, 0, sizeof(tmr));
}

#if 0
static int get_free_tmr(int *groupn, int *idx) {
	int i;

	for(i=0; i < CPU_LAST_TIMER + 1;i++) {
		if (!tmr[i].setup) {
			switch (i) {
				case 0: *groupn = 0; *idx = 0; return 0;
				case 1: *groupn = 0; *idx = 1; return 0;
				case 2: *groupn = 1; *idx = 0; return 0;
				case 3: *groupn = 1; *idx = 1; return 0;
			}
		}
	}

	return -1;
}
#endif

static void IRAM_ATTR get_group_idx(int8_t unit, int *groupn, int *idx) {
	switch (unit) {
		case 0: *groupn = 0; *idx = 0; return;
		case 1: *groupn = 0; *idx = 1; return;
		case 2: *groupn = 1; *idx = 0; return;
		case 3: *groupn = 1; *idx = 1; return;
	}
}

static void alarm_task(void *arg) {
	tmr_alarm_t alarm;

    for(;;) {
        xQueueReceive(queue, &alarm, portMAX_DELAY);

        if (tmr[alarm.unit].callback) {
        	tmr[alarm.unit].callback((void *)((int)alarm.unit));
        }
    }
}

static void IRAM_ATTR isr(void *arg) {
    int unit = (int) arg;
	int groupn, idx;
	portBASE_TYPE high_priority_task_awoken = 0;

	// Get group number / idx
	get_group_idx(unit, &groupn, &idx);

	// Get timer group device
	timg_dev_t *group = (groupn==0?&TIMERG0:&TIMERG1);

	// Check that interrupt is for us
	uint32_t intr_status = group->int_st_timers.val;

    if (intr_status & BIT(idx)) {
    	// Reload alarm value
    	group->hw_timer[idx].update = 1;

    	// Clear inerrupt mask
    	if (idx == 0) {
        	group->int_clr_timers.t0 = 1;
    	} else {
        	group->int_clr_timers.t1 = 1;
    	}

    	// Queue alarm
    	tmr_alarm_t alarm;

    	alarm.unit = unit;

    	if (tmr[alarm.unit].deferred) {
    		xQueueSendFromISR(queue, &alarm, &high_priority_task_awoken);
    	} else {
    		if (tmr[alarm.unit].callback) {
    			tmr[alarm.unit].callback((void *)((int)alarm.unit));
    		}
    	}
    	// Enable alarm again
    	group->hw_timer[idx].config.alarm_en = 1;
    }

    if (high_priority_task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

/*
 * Low-level functions
 *
 */
void tmr_ll_setup(uint8_t unit, uint32_t micros, void(*callback)(void *), uint8_t deferred) {
	int groupn, idx;

	if (tmr[unit].setup) {
		return;
	}

	// For deferred callbacks a queue and a task is needed.
	// In this case in the ISR a message is queued to the queue and callback is
	// executed from a task.
	if (deferred) {
		// Create queue if not created
		if (!queue) {
			queue = xQueueCreate(10, sizeof(tmr_alarm_t));
			if (queue) {
				xTaskCreatePinnedToCore(alarm_task, "tmral", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, NULL, xPortGetCoreID());
			}
		}
	}

	get_group_idx(unit, &groupn, &idx);

	// Configure time and stop
	// Timer is configure for decrement every 1 usec, so
	// alarm valus is exactly the period, expressed in usec
	timer_config_t config;

	config.alarm_en = 1;
	config.auto_reload = 1;
	config.counter_dir = TIMER_COUNT_UP;
	config.divider = 80;
	config.intr_type = TIMER_INTR_LEVEL;
	config.counter_en = TIMER_PAUSE;

    timer_init(groupn, idx, &config);
    timer_pause(groupn, idx);

    // Set timer at required frequency
    timer_set_counter_value(groupn, idx, 0x00000000ULL);
    timer_set_alarm_value(groupn, idx, micros);

    // Enable timer interrupt
    timer_enable_intr(groupn, idx);
    timer_isr_register(groupn, idx, isr, (void *)((int)unit), ESP_INTR_FLAG_IRAM, NULL);

    tmr[unit].setup = 1;
    tmr[unit].callback = callback;
    tmr[unit].deferred = deferred;
}

void tmr_ll_start(uint8_t unit) {
	int groupn, idx;

	get_group_idx(unit, &groupn, &idx);
	timer_start(groupn, idx);
}

void tmr_ll_stop(uint8_t unit) {
	int groupn, idx;

	get_group_idx(unit, &groupn, &idx);
	timer_pause(groupn, idx);
}

/*
 * Operation functions
 */
driver_error_t *tmr_setup(int8_t unit, uint32_t micros, void(*callback)(void *), uint8_t deferred) {
	// Sanity checks
	if ((unit < CPU_FIRST_TIMER) || (unit > CPU_LAST_TIMER)) {
		return driver_operation_error(TIMER_DRIVER, TIMER_ERR_INVALID_UNIT, NULL);
	}

	if (micros < 5) {
		return driver_operation_error(TIMER_DRIVER, TIMER_ERR_INVALID_PERIOD, NULL);
	}

	tmr_ll_setup(unit, micros, callback, deferred);

	return NULL;
}

driver_error_t *tmr_start(int8_t unit) {
	// Sanity checks
	if ((unit < CPU_FIRST_TIMER) || (unit > CPU_LAST_TIMER)) {
		return driver_operation_error(TIMER_DRIVER, TIMER_ERR_INVALID_UNIT, NULL);
	}

	if (!tmr[unit].setup) {
		return driver_operation_error(TIMER_DRIVER, TIMER_ERR_IS_NOT_SETUP, NULL);
	}

	tmr_ll_start(unit);

	return NULL;
}

driver_error_t *tmr_stop(int8_t unit) {
	// Sanity checks
	if ((unit < CPU_FIRST_TIMER) || (unit > CPU_LAST_TIMER)) {
		return driver_operation_error(TIMER_DRIVER, TIMER_ERR_INVALID_UNIT, NULL);
	}

	if (!tmr[unit].setup) {
		return driver_operation_error(TIMER_DRIVER, TIMER_ERR_IS_NOT_SETUP, NULL);
	}

	tmr_ll_stop(unit);

	return NULL;
}

DRIVER_REGISTER(TIMER,timer,NULL,tmr_init,NULL);

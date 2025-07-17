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
 * Lua RTOS, timer driver
 *
 */

#include "sdkconfig.h"
#include "timer.h"

#include "esp_private/periph_ctrl.h"
#include "soc/timer_group_struct.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <math.h>
#include <string.h>

#include "mutex.h"

#include "cpu.h"
#include "timer.h"

// Register driver and messages
static void tmr_init();

DRIVER_REGISTER_BEGIN(TIMER,timer,0,tmr_init,NULL);
	DRIVER_REGISTER_ERROR(TIMER, timer, InvalidUnit, "invalid unit", TIMER_ERR_INVALID_UNIT);
	DRIVER_REGISTER_ERROR(TIMER, timer, NotEnoughtMemory, "not enough memory", TIMER_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(TIMER, timer, NoMoreTimers, "no more timers available", TIMER_ERR_NO_MORE_TIMERS);
	DRIVER_REGISTER_ERROR(TIMER, timer, InvalidPeriod, "invalid period", TIMER_ERR_INVALID_PERIOD);
	DRIVER_REGISTER_ERROR(TIMER, timer, NotSetup, "is not setup", TIMER_ERR_IS_NOT_SETUP);
	DRIVER_REGISTER_ERROR(TIMER, timer, Internal, "internal error", TIMER_ERR_INTERNAL);
DRIVER_REGISTER_END(TIMER,timer,0,tmr_init,NULL);

typedef struct {
	tmr_t timer[CPU_LAST_TIMER + 1]; ///< Timer array with needed information about timers
	xQueueHandle queue;			     ///< Alarm queue for deferred callbacks
	TaskHandle_t task;			     ///< Task handle for deferred callbacks
} tmr_driver_t;

// Driver info
static tmr_driver_t *tmr = NULL;

// Recursive mutex
static SemaphoreHandle_t mtx;

/*
 * Helper functions
 */

static bool timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data) {
	BaseType_t high_priority_task_awoken = pdFALSE;
    tmr_t *cb_timer = (tmr_t *)user_data;

	if (cb_timer->deferred) {
		// Queue alarm
		tmr_alarm_t alarm;

		alarm.unit = cb_timer->unit;

		xQueueSendFromISR(tmr->queue, &alarm, &high_priority_task_awoken);
	} else {
		// Execute callback
		if (cb_timer->callback) {
			cb_timer->callback((void *)((int)cb_timer->unit));
		}
	}

    // return whether we need to yield at the end of ISR
    return (high_priority_task_awoken == pdTRUE);
}

static void inline tmr_lock() {
	xSemaphoreTakeRecursive(mtx, portMAX_DELAY);
}

static void inline tmr_unlock() {
	while (xSemaphoreGiveRecursive(mtx) == pdTRUE);
}

static void tmr_init() {
	mtx = xSemaphoreCreateRecursiveMutex();
}

static int have_timers() {
	int i;

	for(i=0; i < CPU_LAST_TIMER + 1;i++) {
		if (tmr->timer[i].setup) return 1;
	}

	return 0;
}

static void alarm_task(void *arg) {
	tmr_alarm_t alarm;

    for(;;) {
        xQueueReceive(tmr->queue, &alarm, portMAX_DELAY);

        if (tmr->timer[alarm.unit].callback) {
        	tmr->timer[alarm.unit].callback((void *)((int)alarm.unit));
        }
    }
}

/*
 * Low-level functions
 *
 */
int tmr_ll_setup(uint8_t unit, uint32_t micros, void(*callback)(void *), uint8_t deferred) {
	tmr_lock();

	// Allocate space for driver info
	if (!tmr) {
		tmr = calloc(1, sizeof(tmr_driver_t));
		if (!tmr) {
			tmr_unlock();
			return -1;
		}
	}

	if (tmr->timer[unit].setup) {
		tmr_unlock();
		return 0;
	}

	// For deferred callbacks a queue and a task is needed.
	// In this case in the ISR a message is queued to the queue and callback is
	// executed from a task.
	if (deferred) {
		// Create queue if not created
		if (!tmr->queue) {
			tmr->queue = xQueueCreate(10, sizeof(tmr_alarm_t));
			if (!tmr->queue) {
				tmr_unlock();
				return -1;
			}
		}

		// Create task if not created
		if (!tmr->task) {
			BaseType_t xReturn;

			xReturn = xTaskCreatePinnedToCore(alarm_task, "tmral", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &tmr->task, xPortGetCoreID());
			if (xReturn != pdPASS) {
				tmr_unlock();
				return -1;
			}
		}
	}

	// Configure time and stop
	// Timer is configure for increment every 1 usec, so
	// alarm valus is exactly the period, expressed in usec
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    };

    esp_err_t err;

    err = gptimer_new_timer(&timer_config, &tmr->timer[unit].hdnl);
    if (err != ESP_OK) {
    	if (err == ESP_ERR_NO_MEM) {
			tmr_unlock();
			return -1;
    	} else if (err == ESP_ERR_NOT_FOUND) {
			tmr_unlock();
			return -2;
    	} else {
			tmr_unlock();
			return -3;
    	}
    }

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_cb,
    };

    gptimer_register_event_callbacks(tmr->timer[unit].hdnl, &cbs, &tmr->timer[unit]);

    gptimer_enable(tmr->timer[unit].hdnl);

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = micros,
		.flags.auto_reload_on_alarm = 1,
    };

    gptimer_set_alarm_action(tmr->timer[unit].hdnl, &alarm_config);

    tmr->timer[unit].setup = 1;
    tmr->timer[unit].unit = unit;
    tmr->timer[unit].callback = callback;
    tmr->timer[unit].deferred = deferred;

	tmr_unlock();

	return 0;
}

void tmr_ll_unsetup(uint8_t unit) {
	tmr_lock();

	if (!tmr->timer[unit].setup) {
		tmr_unlock();
		return;
	}

	tmr->timer[unit].callback = NULL;
	tmr->timer[unit].deferred = 0;
	tmr->timer[unit].setup = 0;

	// Stop timer
	tmr_ll_stop(unit);

	// Disable timer
	gptimer_disable(tmr->timer[unit].hdnl);

	// Delete timer
	gptimer_del_timer(tmr->timer[unit].hdnl);

	// If we not have timers destroy queue and task if
	// allocated
	if (!have_timers()) {
		if (tmr->task) {
			vTaskDelete(tmr->task);
		}

		if (tmr->queue) {
			vQueueDelete(tmr->queue);
		}

		free(tmr);
		tmr = NULL;
	}

#if 0
	// This fails

	// If we don't have timers in group 0 disable module
	if (!have_timers(0)) {
		periph_module_disable(PERIPH_TIMG0_MODULE);
	}

	// If we don't have timers in group 1 disable module
	if (!have_timers(1)) {
		periph_module_disable(PERIPH_TIMG1_MODULE);
	}
#endif

	tmr_unlock();
}

void tmr_ll_start(uint8_t unit) {
	tmr_lock();
	gptimer_start(tmr->timer[unit].hdnl);
	tmr_unlock();
}

void tmr_ll_stop(uint8_t unit) {
	tmr_lock();
	gptimer_stop(tmr->timer[unit].hdnl);
	tmr_unlock();
}

/*
 * Operation functions
 */
driver_error_t *tmr_setup(int8_t unit, uint32_t micros, void(*callback)(void *), uint8_t deferred) {
	// Sanity checks
	if ((unit < CPU_FIRST_TIMER) || (unit > CPU_LAST_TIMER)) {
		return driver_error(TIMER_DRIVER, TIMER_ERR_INVALID_UNIT, NULL);
	}

	if (micros < 5) {
		return driver_error(TIMER_DRIVER, TIMER_ERR_INVALID_PERIOD, NULL);
	}

	int err = tmr_ll_setup(unit, micros, callback, deferred);
	if (err == -1) {
		return driver_error(TIMER_DRIVER, TIMER_ERR_NOT_ENOUGH_MEMORY, NULL);
	} else if (err == -2) {
		return driver_error(TIMER_DRIVER, TIMER_ERR_NO_MORE_TIMERS, NULL);
	} else if (err == -3) {
		return driver_error(TIMER_DRIVER, TIMER_ERR_INTERNAL, NULL);
	}

	return NULL;
}

driver_error_t *tmr_unsetup(int8_t unit) {
	// Sanity checks
	if ((unit < CPU_FIRST_TIMER) || (unit > CPU_LAST_TIMER)) {
		return driver_error(TIMER_DRIVER, TIMER_ERR_INVALID_UNIT, NULL);
	}

	tmr_ll_unsetup(unit);

	return NULL;
}

driver_error_t *tmr_start(int8_t unit) {
	// Sanity checks
	if ((unit < CPU_FIRST_TIMER) || (unit > CPU_LAST_TIMER)) {
		return driver_error(TIMER_DRIVER, TIMER_ERR_INVALID_UNIT, NULL);
	}

	if (!tmr->timer[unit].setup) {
		return driver_error(TIMER_DRIVER, TIMER_ERR_IS_NOT_SETUP, NULL);
	}

	tmr_ll_start(unit);

	return NULL;
}

driver_error_t *tmr_stop(int8_t unit) {
	// Sanity checks
	if ((unit < CPU_FIRST_TIMER) || (unit > CPU_LAST_TIMER)) {
		return driver_error(TIMER_DRIVER, TIMER_ERR_INVALID_UNIT, NULL);
	}

	if (!tmr->timer[unit].setup) {
		return driver_error(TIMER_DRIVER, TIMER_ERR_IS_NOT_SETUP, NULL);
	}

	tmr_ll_stop(unit);

	return NULL;
}

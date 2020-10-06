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
 * Lua RTOS, virtual timer driver
 *
 */

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_attr.h"
#include "timer.h"
#include "vtimer.h"

#include <stdlib.h>
#include <stdio.h>

#include <sys/pool.h>
#include <sys/sorted_list.h>

/* DEFINES */
/* ------- */
#define VTMR_UNIT 0
#define TIMER_GROUP_VTIMER 1
#define TIMER_VTIMER 0
#define VTIMER_CORRECTION 14
#define VTMR_START_Q_SIZE 10
#define VTMR_STOP_Q_SIZE 10

#ifndef TG1_WDT_TICK_US
#define TG1_WDT_TICK_US 500
#endif

#ifndef WDT_INT_NUM
#define WDT_INT_NUM 24
#endif

/* TYPES */
/* ----- */
typedef struct {
  uint32_t id;
  uint64_t when;
  vtmr_callback_t callback;
  void *args;
} vtmr_start_t;

typedef struct {
  uint32_t id;
} vtmr_stop_t;

typedef struct {
  uint32_t id;
  uint64_t when;
  uint8_t is_stop;
  vtmr_callback_t callback;
  void *args;
  uint8_t pool_index;
} vtmr_t;

static sorted_list_t *timer = NULL;
static mem_pool_t *pool;
static QueueHandle_t start_queue = NULL;
static QueueHandle_t stop_queue = NULL;
static uint8_t in_isr = false;

/* INTERNAL FUNCTIONS */
/* ------------------ */
static void _schelude_next();
static void _process();
static void _stop_timers(uint8_t delete_from_list);

static int8_t _cmp(vtmr_t *timer1, vtmr_t *timer2);
static void _isr(void *args);


// Register driver and messages
static void vtmr_init();

DRIVER_REGISTER_BEGIN(VTIMER,vtimer,0,vtmr_init,NULL);
	DRIVER_REGISTER_ERROR(VTIMER, vtimer, InvalidUnit, "invalid callback", VTIMER_ERR_INVALID_CALLBACK);
DRIVER_REGISTER_END(VTIMER,v,0,vtmr_init,NULL);



static void vtmr_init() {
  // Create pool
  assert(pool_setup(32, sizeof(vtmr_t), &pool) == 0);

  // Create start / stop queues
  start_queue = xQueueCreate(VTMR_START_Q_SIZE, sizeof(vtmr_start_t));
  assert(start_queue != NULL);

  stop_queue = xQueueCreate(VTMR_STOP_Q_SIZE, sizeof(vtmr_stop_t));
  assert(stop_queue != NULL);

  // Create a list to sort the active timers
  timer = calloc(1, sizeof(sorted_list_t));
  assert(timer != NULL);

  assert(sorted_list_setup_static(timer, (sorted_list_cmp_func_t)_cmp) == 0);

  // Configure timer, as not autoload
  assert(tmr_setup(VTMR_UNIT, 0, _isr, 0) == NULL);
}

static void IRAM_ATTR _add_timers() {
  sorted_list_iterator_t iterator;
  vtmr_start_t start;
  vtmr_t *vtmr;

  while (xQueueReceiveFromISR(start_queue, &start, NULL) == pdTRUE) {
    // Get a free virtual timer from pool
    uint8_t item_id;

    vtmr = (vtmr_t *)pool_get(pool, &item_id);
    assert(vtmr != NULL);

    // Add virtual timer to virtual timer list
    vtmr->id = start.id;
    vtmr->when = start.when;
    vtmr->callback = start.callback;
    vtmr->args = start.args;
    vtmr->pool_index = item_id;
    vtmr->is_stop = false;

    sorted_list_iterate_fast(timer, &iterator);
    sorted_list_add(timer, vtmr, &iterator);
  }
}

static void IRAM_ATTR _stop_timers(uint8_t delete_from_list) {
  vtmr_stop_t stop;

  sorted_list_iterator_t iterator;
  vtmr_t *current = NULL;

  while (xQueueReceiveFromISR(stop_queue, &stop, NULL) == pdTRUE) {
    sorted_list_iterate_fast(timer, &iterator);
    sorted_list_next_fast(&iterator, (void **)&current);

    while (current != NULL) {
      if (current->id == stop.id) {
        if (delete_from_list) {
          // Remove current virtual timer from virtual timer list
          sorted_list_remove_fast(&iterator);

          // Remove virtual timer from pool
          pool_free(pool, current->pool_index);
        } else {
          current->is_stop = true;
        }
      }

      sorted_list_next_fast(&iterator, (void **)&current);
    }
  }
}

static void IRAM_ATTR _process() {
  sorted_list_iterator_t iterator;
  vtmr_t *current = NULL;

  // Stop timers
  _stop_timers(true);

  // Add new virtual timers
  _add_timers();

  // Get first virtual timer on list, it is the virtual timer who start the HW timer
  sorted_list_iterate_fast(timer, &iterator);
  sorted_list_next_fast(&iterator, (void **)&current);

  if (current != NULL) {
    // Process all expired virtual timers
    while (current != NULL) {
      if (current->is_stop) {
        // Remove current virtual timer from virtual timer list
        sorted_list_remove_fast(&iterator);

        // Remove virtual timer from pool
        pool_free(pool, current->pool_index);
      } else if (current->when <= esp_timer_get_time()) {
        // Call callback
        current->callback(current->args);

        // Remove current virtual timer from virtual timer list
        sorted_list_remove_fast(&iterator);

        // Remove virtual timer from pool
        pool_free(pool, current->pool_index);

        // Stop timers
        _stop_timers(false);
      } else {
        break;
      }

      sorted_list_next_fast(&iterator, (void **)&current);
    }
  }
}

static void IRAM_ATTR _schelude_next() {
  sorted_list_iterator_t iterator;
  vtmr_t *next_timer = NULL;

  // Add new virtual timers
  _add_timers();

  // Get first virtual timer not stopped on list, it is the next virtual timer
  sorted_list_iterate_fast(timer, &iterator);
  sorted_list_next_fast(&iterator, (void **)&next_timer);

  while ((next_timer != NULL) && (next_timer->is_stop)) {
    // Remove current virtual timer from virtual timer list
    sorted_list_remove_fast(&iterator);

    // Remove virtual timer from pool
    pool_free(pool, next_timer->pool_index);

    sorted_list_next_fast(&iterator, (void **)&next_timer);
  }

  if (next_timer != NULL) {
    uint64_t now = esp_timer_get_time();

    if (next_timer->when <= now) {
      tmr_ll_trigger(VTMR_UNIT);
    } else {
      uint32_t pending = next_timer->when - now;

      tmr_ll_start_in(VTMR_UNIT, (pending > VTIMER_CORRECTION) ? pending - VTIMER_CORRECTION : pending);
    }
  }
}

static void IRAM_ATTR _isr(void *args) {
  in_isr = true;

  // Process virtual timers
  _process();

  // Schelude next virtual timer
  _schelude_next();

  in_isr = false;
}

static int8_t IRAM_ATTR _cmp(vtmr_t *timer1, vtmr_t *timer2) {
  if (timer1->when == timer2->when) {
    return 0;
  } else if (timer1->when < timer2->when) {
    return -1;
  }

  return 1;
}

static void IRAM_ATTR _start_timer_internal(uint32_t id, uint64_t when, vtmr_callback_t callback, void *args) {
  // Get current time
  uint64_t now = esp_timer_get_time();

  // Populate timer start information
  vtmr_start_t start;

  start.when = now + when;
  start.callback = callback;
  start.args = args;
  start.id = id;

  // Queue start of virtual timer
  if (xPortInIsrContext()) {
    configASSERT(xQueueSendFromISR(start_queue, &start, NULL) == pdTRUE);
  } else {
    xQueueSend(start_queue, &start, portMAX_DELAY);
  }

  // If timer has been started outside a timer callback, we need to trigger timer
  // programatically to insert the virtual timer in timer list.
  //
  // If timer has been started inside a timer callback the timer is immediately
  // inserted in timer list inside the timer isr, it is not required to trigger timer.
  if (!in_isr) {
    tmr_ll_trigger(VTMR_UNIT);
  }
}

static void IRAM_ATTR _stop_timer_internal(uint32_t id) {
  // Populate timer stop information
  vtmr_stop_t stop;

  stop.id = id;

  // Queue stop of virtual timer
  if (xPortInIsrContext()) {
    configASSERT(xQueueSendFromISR(stop_queue, &stop, NULL) == pdTRUE);
  } else {
    xQueueSend(stop_queue, &stop, portMAX_DELAY);
  }

  // If timer has been stopped outside a timer callback, we need to trigger timer
  // programatically to stop the virtual timer.
  //
  // If timer has been stopped inside a timer callback the timer is immediately
  // stopped inside the timer isr, it is not required to trigger timer.
  if (!in_isr) {
	  tmr_ll_trigger(VTMR_UNIT);
  }
}

/*
 * Operation functions
 */

driver_error_t IRAM_ATTR *vtmr_start(uint64_t when, vtmr_callback_t callback, void *args) {
  // Sanity checks
  if (callback == NULL) {
	  return driver_error(VTIMER_DRIVER, VTIMER_ERR_INVALID_CALLBACK, NULL);
  }

  _start_timer_internal((uint32_t)callback, when, callback, args);

  return NULL;
}

driver_error_t IRAM_ATTR *vtmr_stop(vtmr_callback_t callback) {
  // Sanity checks
  if (callback == NULL) {
	  return driver_error(VTIMER_DRIVER, VTIMER_ERR_INVALID_CALLBACK, NULL);
  }

  _stop_timer_internal((uint32_t)callback);

  return NULL;
}

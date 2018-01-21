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
 * Lua RTOS, timer driver
 *
 */

#ifndef TIMER_H
#define	TIMER_H

#include "driver/timer.h"

#include <stdint.h>

#include <sys/driver.h>

typedef struct{
	int8_t unit;
} tmr_alarm_t;

typedef struct {
	uint8_t setup;
	void (*callback)(void *);
	uint8_t deferred;
	timer_isr_handle_t isrh;
} tmr_t;

typedef void(*tmr_isr_t)(void *);

// TIMER errors
#define TIMER_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(TIMER_DRIVER_ID) |  0)
#define TIMER_ERR_NOT_ENOUGH_MEMORY        (DRIVER_EXCEPTION_BASE(TIMER_DRIVER_ID) |  1)
#define TIMER_ERR_NO_MORE_TIMERS           (DRIVER_EXCEPTION_BASE(TIMER_DRIVER_ID) |  2)
#define TIMER_ERR_INVALID_PERIOD		   (DRIVER_EXCEPTION_BASE(TIMER_DRIVER_ID) |  3)
#define TIMER_ERR_IS_NOT_SETUP		   	   (DRIVER_EXCEPTION_BASE(TIMER_DRIVER_ID) |  4)

/**
 * @brief Configures a timer. After timer is configured you must start timer using
 * 		  tmr_start function.  No sanity checks are done (use only in driver develop).
 *
 * @param unit Hardware timer, from 0 to 3.
 * @param micros Period of timer, in microseconds.
 * @param callback Callback function to call every micros period.
 * @param deferred If 0, the callback are executed in the isr. If 1, the callback
 *                 is deferred and is called outside the interrupt. Non deferred
 *                 callbacks must reside in IRAM.
 *
 * @return 0 if success, -1 if error (memory error)
 *
 */
int tmr_ll_setup(uint8_t unit, uint32_t micros, void(*callback)(void *), uint8_t deferred);

/**
 * @brief Removes a timer and the resources that uses.
 * 		  tmr_start function.  No sanity checks are done (use only in driver develop).
 *
 * @param unit Hardware timer, from 0 to 3.
 */
void tmr_ll_unsetup(uint8_t unit);

/**
 * @brief Start a previous configured timer. No sanity checks are done (use only in driver develop).
 *
 * @param unit Hardware timer, from 0 to 3.
 */
void tmr_ll_start(uint8_t unit);

/**
 * @brief Stop a previous configured timer. No sanity checks are done (use only in driver develop).
 *
 * @param unit Hardware timer, from 0 to 3.
 */
void tmr_ll_stop(uint8_t unit);

/**
 * @brief Configures a timer. After timer is configured you must start timer using
 * 		  tmr_start function.
 *
 * @param unit Hardware timer, from 0 to 3.
 * @param micros Period of timer, in microseconds.
 * @param callback Callback function to call every micros period.
 * @param deferred If 0, the callback are executed in the isr. If 1, the callback
 *                 is deferred and is called outside the interrupt. Non deferred
 *                 callbacks must reside in IRAM.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 TIMER_ERR_INVALID_UNIT
 *     	 TIMER_ERR_INVALID_PERIOD
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 *     	 SPI_ERR_NOT_ENOUGH_MEMORY
 */
driver_error_t *tmr_setup(int8_t unit, uint32_t micros, void(*callback)(void *), uint8_t deferred);

/**
 * @brief Removes a timer and the resources that uses.
 * 		  tmr_start function.  No sanity checks are done (use only in driver develop).
 *
 * @param unit Hardware timer, from 0 to 3.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 TIMER_ERR_INVALID_UNIT
 */
driver_error_t *tmr_unsetup(int8_t unit);

/**
 * @brief Start a previous configured timer.
 *
 * @param unit Hardware timer, from 0 to 3.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 TIMER_ERR_IS_NOT_SETUP
 */
driver_error_t *tmr_start(int8_t unit);

/**
 * @brief Stop a previous configured timer.
 *
 * @param unit Hardware timer, from 0 to 3.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 TIMER_ERR_IS_NOT_SETUP
 */
driver_error_t *tmr_stop(int8_t unit);

void get_group_idx(int8_t unit, int *groupn, int *idx);

#endif	/* TIMER_H */

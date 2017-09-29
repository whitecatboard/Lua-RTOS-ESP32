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

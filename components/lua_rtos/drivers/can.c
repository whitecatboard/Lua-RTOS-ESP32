/*
 * Lua RTOS, CAN driver
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

#if CONFIG_LUA_RTOS_LUA_USE_CAN

#include <stdint.h>
#include <string.h>

#include <can_bus.h>

#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/can.h>
#include <drivers/gpio.h>

static uint8_t setup = 0;

// CAN configuration
CAN_device_t CAN_cfg;

// Driver message errors
DRIVER_REGISTER_ERROR(CAN, can, NotEnoughtMemory, "not enough memory", CAN_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_ERROR(CAN, can, InvalidFrameLength, "invalid frame length", CAN_ERR_INVALID_FRAME_LENGTH);
DRIVER_REGISTER_ERROR(CAN, can, InvalidUnit, "invalid unit", CAN_ERR_INVALID_UNIT);
DRIVER_REGISTER_ERROR(CAN, can, NoMoreFiltersAllowed, "no more filters allowed", CAN_ERR_NO_MORE_FILTERS_ALLOWED);
DRIVER_REGISTER_ERROR(CAN, can, InvalidFilter, "invalid filter", CAN_ERR_INVALID_FILTER);
DRIVER_REGISTER_ERROR(CAN, can, NotSetup, "is not setup", CAN_ERR_IS_NOT_SETUP);

/*
 * Helper functions
 */

/*
 * Operation functions
 */

driver_error_t *can_setup(int32_t unit, uint16_t speed) {
	driver_unit_lock_error_t *lock_error = NULL;

	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if (!setup) {
	    // Lock TX pin
	    if ((lock_error = driver_lock(CAN_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_CAN_TX, DRIVER_ALL_FLAGS, "TX"))) {
	    	// Revoked lock on pin
	    	return driver_lock_error(CAN_DRIVER, lock_error);
	    }

	    // Lock RX pin
	    if ((lock_error = driver_lock(CAN_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_CAN_RX, DRIVER_ALL_FLAGS, "RX"))) {
	    	// Revoked lock on pin
	    	return driver_lock_error(CAN_DRIVER, lock_error);
	    }
	}

	// Set CAN configuration
	CAN_cfg.speed = speed;
	CAN_cfg.tx_pin_id = CONFIG_LUA_RTOS_CAN_TX;
	CAN_cfg.rx_pin_id = CONFIG_LUA_RTOS_CAN_RX;

	if (!setup) {
		CAN_cfg.rx_queue = xQueueCreate(50,sizeof(CAN_frame_t));;

		if (!CAN_cfg.rx_queue) {
			return driver_error(CAN_DRIVER, CAN_ERR_NOT_ENOUGH_MEMORY, NULL);
		}
	}

	// Start CAN module
	CAN_init();

	if (!setup) {
		syslog(LOG_INFO, "can%d at pins tx=%s%d, rx=%s%d", 0,
			gpio_portname(CONFIG_LUA_RTOS_CAN_TX), gpio_name(CONFIG_LUA_RTOS_CAN_TX),
			gpio_portname(CONFIG_LUA_RTOS_CAN_RX), gpio_name(CONFIG_LUA_RTOS_CAN_RX)
		);
	}

	setup = 1;

	return NULL;
}

driver_error_t *can_tx(int32_t unit, uint32_t msg_id, uint8_t msg_type, uint8_t *data, uint8_t len) {
	CAN_frame_t frame;

	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if (len > 8) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_FRAME_LENGTH, NULL);
	}

	if (!setup) {
		return driver_error(CAN_DRIVER, CAN_ERR_IS_NOT_SETUP, NULL);
	}

	// Populate frame
	frame.MsgID = msg_id;
	frame.DLC = len;
	memcpy(&frame.data, data, len);

	// TX
	CAN_write_frame(&frame);

	return NULL;
}

driver_error_t *can_rx(int32_t unit, uint32_t *msg_id, uint8_t *msg_type, uint8_t *data, uint8_t *len) {
	CAN_frame_t frame;

	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if (!setup) {
		return driver_error(CAN_DRIVER, CAN_ERR_IS_NOT_SETUP, NULL);
	}

	// Read next frame
	xQueueReceive(CAN_cfg.rx_queue, &frame, portMAX_DELAY);

	*msg_id = frame.MsgID;
	*msg_type = 0;
	*len = frame.DLC;
	memcpy(data, &frame.data, frame.DLC);

	return NULL;
}

driver_error_t *can_add_filter(int32_t unit, int32_t fromId, int32_t toId) {
	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if ((fromId < 0) || (toId < 0)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_FILTER, "must be >= 0");
	}

	if ((fromId > toId)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_FILTER, "from filter must be >= to filter");
	}

	if (!setup) {
		return driver_error(CAN_DRIVER, CAN_ERR_IS_NOT_SETUP, NULL);
	}

	if (CAN_add_filter(fromId, toId)) {
		return driver_error(CAN_DRIVER, CAN_ERR_NO_MORE_FILTERS_ALLOWED, NULL);
	}

	return NULL;
}

driver_error_t *can_remove_filter(int32_t unit, int32_t fromId, int32_t toId) {
	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if ((fromId < 0) || (toId < 0)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_FILTER, "must be >= 0");
	}

	if ((fromId > toId)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_FILTER, "from filter must be >= to filter");
	}

	if (!setup) {
		return driver_error(CAN_DRIVER, CAN_ERR_IS_NOT_SETUP, NULL);
	}

	if (CAN_remove_filter(fromId, toId)) {
		return driver_error(CAN_DRIVER, CAN_ERR_NO_MORE_FILTERS_ALLOWED, NULL);
	}

	return NULL;
}

DRIVER_REGISTER(CAN,can,NULL,NULL,NULL);

#endif

/*
 * Lua RTOS, Tilt switch sensor (example SW-520D)
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

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_TILT_SWITCH

#include <drivers/sensor.h>

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) tilt_switch_sensor = {
	.id = "TILT_SWITCH",
	.interface = {
		{
			.type = GPIO_INTERFACE,

			// 1000: debouncing threshold period
			.flags = (SENSOR_FLAG_ON_OFF | SENSOR_FLAG_ON_L | SENSOR_FLAG_DEBOUNCING) | (100000 << 16)
		},
	},
	.data = {
		{.id = "on", .type = SENSOR_DATA_INT},
	}
};

#endif
#endif

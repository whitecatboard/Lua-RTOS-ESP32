/*
 * Lua RTOS, Push switch sensor
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

#include <drivers/sensor.h>
//#if CONFIG_LUA_RTOS_USE_SENSOR_PUSH_SWITCH

driver_error_t *_3_pos_switch_setup(sensor_instance_t *unit);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) _3_pos_switch_sensor = {
	.id = "3P_TOGGLE_SWITCH",
	.interface = {
		{
			.type = GPIO_INTERFACE,

			.flags = SENSOR_FLAG_AUTO_ACQ | SENSOR_FLAG_ON_OFF | SENSOR_FLAG_ON_H(0) | SENSOR_FLAG_ON_L(1) |
					 SENSOR_FLAG_DEBOUNCING | SENSOR_FLAG_DEBOUNCING_THRESHOLD(10000)
		},
		{
			.type = GPIO_INTERFACE,

			.flags = SENSOR_FLAG_AUTO_ACQ | SENSOR_FLAG_ON_OFF | SENSOR_FLAG_ON_H(0) | SENSOR_FLAG_ON_L(2) |
					 SENSOR_FLAG_DEBOUNCING | SENSOR_FLAG_DEBOUNCING_THRESHOLD(10000)
		},
	},
	.interface_name = {"P1", "P2"},
	.data = {
		{.id = "pos", .type = SENSOR_DATA_INT},
	},
	.setup = _3_pos_switch_setup
};

driver_error_t *_3_pos_switch_setup(sensor_instance_t *unit) {
	// Get initial state
	if (gpio_ll_pin_get(unit->setup[0].gpio.gpio) == 0) {
		unit->data[0].integerd.value = SENSOR_FLAG_GET_ON_L(unit->sensor->interface[0]);
	} else if (gpio_ll_pin_get(unit->setup[1].gpio.gpio) == 0) {
		unit->data[0].integerd.value = SENSOR_FLAG_GET_ON_L(unit->sensor->interface[1]);
	}

	unit->latch[0].value.integerd.value = unit->data[0].integerd.value;

	return NULL;
}

//#endif
#endif

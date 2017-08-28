/*
 * Lua RTOS, Relative rotary encoder sensor
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
#if CONFIG_LUA_RTOS_USE_SENSOR_RELATIVE_ROTARY_ENCODER

#include "esp_attr.h"

#include <string.h>

#include <sys/mutex.h>

#include <drivers/sensor.h>
#include <drivers/encoder.h>

driver_error_t *relative_rotary_encoder_setup(sensor_instance_t *unit);
driver_error_t *relative_rotary_encoder_unsetup(sensor_instance_t *unit);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) relative_rotary_encoder_sensor = {
	.id = "REL_ROT_ENCODER",
	.interface = {
		GPIO_INTERFACE,
		GPIO_INTERFACE,
		GPIO_INTERFACE,
	},
	.interface_name = {
		"A",
		"B",
		"SW"
	},
	.flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ),
	.data = {
		{.id = "dir", .type = SENSOR_DATA_INT},
		{.id = "val", .type = SENSOR_DATA_INT},
		{.id = "sw",  .type = SENSOR_DATA_INT},
	},
	.setup = relative_rotary_encoder_setup,
	.unsetup = relative_rotary_encoder_unsetup,
};

/*
 * Helper functions
 */
static void IRAM_ATTR callback_func(int callback, int8_t dir, uint32_t counter, uint8_t button) {
	sensor_instance_t *unit = (sensor_instance_t *)callback;

	mtx_lock(&unit->mtx);

	// Latch values
	memcpy(unit->latch, unit->data, sizeof(unit->data));

	// Store current data
	unit->data[0].integerd.value = dir;
	unit->data[1].integerd.value = counter;
	unit->data[2].integerd.value = button;

	if (counter != unit->latch[1].integerd.value) {
		// Encoder is moving
		unit->latch[0].integerd.value = 0;
	}

	sensor_queue_callbacks(unit);

	mtx_unlock(&unit->mtx);
}

/*
 * Operation functions
 */
driver_error_t *relative_rotary_encoder_setup(sensor_instance_t *unit) {
	driver_error_t *error;
	encoder_h_t *h;

	// Setup encoder
	error = encoder_setup(unit->setup[0].gpio.gpio, unit->setup[1].gpio.gpio, unit->setup[2].gpio.gpio, &h);
	if (error) {
		return error;
	}

	// Store encoder handler
	unit->unit = (int)h;

	// Install callback, not deferred
	error = encoder_register_callback(h, callback_func, (int)unit, 0);
	if (error) {
		encoder_unsetup(h);
		return error;
	}

	return NULL;
}

driver_error_t *relative_rotary_encoder_unsetup(sensor_instance_t *unit) {
	driver_error_t *error;

	error = encoder_unsetup((encoder_h_t *)unit->unit);
	if (error){
		return NULL;
	}

	return NULL;
}
#endif
#endif

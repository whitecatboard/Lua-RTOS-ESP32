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
 * Lua RTOS, Relative rotary encoder sensor
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
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
	},
	.interface_name = {"A", "B", "SW"},
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
	unit->latch[0].value.raw.value = unit->data[0].raw.value;
	unit->latch[1].value.raw.value = unit->data[1].raw.value;
	unit->latch[2].value.raw.value = unit->data[2].raw.value;

	// Store current data
	unit->data[0].integerd.value = dir;
	unit->data[1].integerd.value = counter;
	unit->data[2].integerd.value = button;

	if (counter != unit->latch[1].value.integerd.value) {
		// Encoder is moving
		// dir is -1 or 1, we must clear latch for ensure that callback on dir
		// property willbe called
		unit->latch[0].value.integerd.value = 0;
	}

	sensor_queue_callbacks(unit, 0, 2);

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

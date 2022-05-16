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
 * Lua RTOS, BDE0600G sensor (Temperature)
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_BDE0600G

#define BDE0600G_SAMPLES 10

#include <math.h>
#include <string.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

driver_error_t *BDE0600G_presetup(sensor_instance_t *unit);
driver_error_t *BDE0600G_acquire(sensor_instance_t *unit, sensor_value_t *values);
driver_error_t *BDE0600G_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) BDE0600G_sensor = {
	.id = "BDE0600G",
	.interface = {
		{.type = ADC_INTERFACE},
	},
	.data = {
			{.id = "temperature", .type = SENSOR_DATA_FLOAT},
	},
	.properties = {
		{.id = "calibration",.type = SENSOR_DATA_FLOAT},
	},
	.presetup = BDE0600G_presetup,
	.acquire = BDE0600G_acquire,
	.set = BDE0600G_set
};

/*
 * Operation functions
 */

driver_error_t *BDE0600G_presetup(sensor_instance_t *unit) {
	unit->setup[0].adc.max = CONFIG_LUA_RTOS_VDD;

	unit->properties[0].integerd.value = 0; // Calibration = 0

	return NULL;
}

driver_error_t *BDE0600G_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	double mvolts;
	double volts;

	// Read average for some samples
	if ((error = adc_read_avg(&unit->setup[0].adc.h, BDE0600G_SAMPLES, NULL, &mvolts))) {
		return error;
	}

	// Convert mvolts to volts
	volts = mvolts / 1000.0;

	// Calculate and round to 2 decimals
	values[0].floatd.value = roundf(100 * (((volts-1.753)/(-0.01068)) + 30)) / 100;
	values[0].floatd.value += unit->properties[0].floatd.value;

	return NULL;
}

driver_error_t *BDE0600G_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting) {
	if (strcmp(id,"calibration") == 0) {
			memcpy(&unit->properties[0], setting, sizeof(sensor_value_t));
	}

	return NULL;
}

#endif
#endif

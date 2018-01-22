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
 * Lua RTOS, GUVA-S12SD sensor (ultraviolet radiation)
 */

/*
 * Extracted from https://www.mysensors.org/build/uv
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_GUVA_S12SD

#define GUVA_S12SD_SAMPLES 10

#include <math.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

static const uint16_t uvIndexValue [12] = { 50, 227, 318, 408, 503, 606, 696, 795, 881, 976, 1079, 1170};

driver_error_t *guva_s12sd_presetup(sensor_instance_t *unit);
driver_error_t *guva_s12sd_acquire(sensor_instance_t *unit, sensor_value_t *values);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) guva_s12sd_sensor = {
	.id = "GUVA-S12SD",
	.interface = {
		{.type = ADC_INTERFACE},
	},
	.data = {
		{.id = "UV Index", .type = SENSOR_DATA_FLOAT},
	},
	.presetup = guva_s12sd_presetup,
	.acquire = guva_s12sd_acquire
};

/*
 * Operation functions
 */

driver_error_t *guva_s12sd_presetup(sensor_instance_t *unit) {
	// Sensor vout is <= 1100 mVols

	unit->setup[0].adc.max = 1100;

	return NULL;
}

driver_error_t *guva_s12sd_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	double mvolts;
	int i;

	// Read average for some samples
	if ((error = adc_read_avg(&unit->setup[0].adc.h, GUVA_S12SD_SAMPLES, NULL, &mvolts))) {
		return error;
	}

	// Calculate uv index
	float uvIndex;

	// Find index into uv index table
	for (i = 0; i < 12; i++) {
		if (mvolts <= uvIndexValue[i]) {
			uvIndex = i;
			break;
		}
	}

	// Calculate decimal, if possible
	if (i > 0) {
		float vRange = uvIndexValue[i] - uvIndexValue[i-1];
		float vCalc = mvolts - uvIndexValue[i-1];
		uvIndex += (1.0/vRange) * vCalc - 1.0;
	}

	// Round to 1 decimal place
	values[0].floatd.value = roundf(10 * uvIndex) / 10;

	return NULL;
}

#endif
#endif

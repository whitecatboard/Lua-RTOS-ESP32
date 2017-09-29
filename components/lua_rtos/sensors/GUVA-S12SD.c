/*
 * Lua RTOS, GUVA-S12SD sensor (ultraviolet radiation)
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

static uint16_t uvIndexValue [12] = { 50, 227, 318, 408, 503, 606, 696, 795, 881, 976, 1079, 1170};

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

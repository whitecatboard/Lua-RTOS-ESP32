/*
 * Lua RTOS, LM35 sensor (temperature)
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

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_LM35

#define lm35_SAMPLES 10

#include <math.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

driver_error_t *lm35_presetup(sensor_instance_t *unit);
driver_error_t *lm35_acquire(sensor_instance_t *unit, sensor_value_t *values);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) lm35_sensor = {
	.id = "LM35",
	.interface = {
		{.type = ADC_INTERFACE},
	},
	.data = {
		{.id = "temperature", .type = SENSOR_DATA_FLOAT},
	},
	.presetup = lm35_presetup,
	.acquire = lm35_acquire
};

/*
 * Operation functions
 */
driver_error_t *lm35_presetup(sensor_instance_t *unit) {
	unit->setup[0].adc.max = 1500;

	return NULL;
}

driver_error_t *lm35_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	double mvolts = 0;

	// Read average for some samples
	if ((error = adc_read_avg(&unit->setup[0].adc.h, lm35_SAMPLES, NULL, &mvolts))) {
		return error;
	}

	// Calculate temperature
	// Round to 1 decimal place
	values->floatd.value = floor((float)10.0 * (((float)mvolts) / (float)10.0)) / (float)10.0;

	return NULL;
}

#endif
#endif

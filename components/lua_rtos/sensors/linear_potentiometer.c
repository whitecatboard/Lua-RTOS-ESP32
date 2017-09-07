/*
 * Lua RTOS, Potentiometer sensor
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
#if CONFIG_LUA_RTOS_USE_SENSOR_POT

#define POTENTIOMETER_SAMPLES 10

#include <math.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

driver_error_t *potentiometer_acquire(sensor_instance_t *unit, sensor_value_t *values);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) potentiometer_sensor = {
	.id = "LINEAR_POT",
	.interface = {
		{.type = ADC_INTERFACE},
	},
	.data = {
		{.id = "val", .type = SENSOR_DATA_FLOAT},
	},
	.acquire = potentiometer_acquire
};

/*
 * Operation functions
 */
driver_error_t *potentiometer_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	double mvolts = 0;

	// Read average for some samples
	if ((error = adc_read_avg(&unit->setup[0].adc.h, POTENTIOMETER_SAMPLES, NULL, &mvolts))) {
		return error;
	}

	// Get channel info
	adc_channel_t *chan;

	adc_get_channel(&unit->setup[0].adc.h, &chan);

	// Estimate POT value (2 decimals)
	values[0].floatd.value = roundf(100 * (mvolts / CONFIG_LUA_RTOS_VDD)) / 100;

	if (values[0].floatd.value > 1.0) {
		values[0].floatd.value = 1.0;
	}

	return NULL;
}

#endif
#endif

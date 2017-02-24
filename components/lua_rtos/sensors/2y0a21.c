/*
 * Lua RTOS, 2Y0A21 sensor (proximity)
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
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

#if 0

#include "2y0a21.h"

#include <math.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) s2y0a21_sensor = {
	.id = "2Y0A21",
	.interface = ADC_INTERFACE,
	.data = {
		{.id = "distance", .type = SENSOR_DATA_FLOAT},
	},
	.setup = s2y0a21_setup,
	.acquire = s2y0a21_acquire
};

/*
 * Operation functions
 */
driver_error_t *s2y0a21_setup(sensor_instance_t *unit) {
	driver_error_t *error;

	// Sensor out is from 0V to 3.3V
	// ESP32 ADC reference is 1.1V
	// We need to attenuate ADC input by 11db 1/3.6
	if ((error = adc_setup_channel(unit->setup.adc.channel, unit->setup.adc.resolution, ADC_ATTEN_11db))) {
		return error;
	}

	return NULL;
}

driver_error_t *s2y0a21_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	int raw = 0;
	double mvolts = 0;

	// Read value
	if ((error = adc_read(unit->setup.adc.channel, &raw, &mvolts))) {
		return error;
	}

	mvolts = mvolts * 3.6;

	// Calculate distance
	values->floatd.value = 29.988 * pow(mvolts / 1000.0, -1.173);

	return NULL;
}

#endif

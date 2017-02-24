/*
 * Lua RTOS, TMP36 sensor (temperature)
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

#include "tmp36.h"

#include <math.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) tmp36_sensor = {
	.id = "TMP36",
	.interface = ADC_INTERFACE,
	.data = {
		{.id = "temperature", .type = SENSOR_DATA_FLOAT},
	},
	.setup = tmp36_setup,
	.acquire = tmp36_acquire
};

/*
 * Operation functions
 */
driver_error_t *tmp36_setup(sensor_instance_t *unit) {
	return NULL;
}

driver_error_t *tmp36_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	int raw = 0;
	double mvolts = 0;

	// Read value
	if ((error = adc_read(unit->setup.adc.unit, unit->setup.adc.channel, &raw, &mvolts))) {
		return error;
	}

	// Calculate temperature
	// TMP36 has a resolution of 0.5 ºC, so round to 1 decimal place
	values->floatd.value = floor(10.0 * (((float)mvolts - 500) / 10)) / 10.0;

	return NULL;
}

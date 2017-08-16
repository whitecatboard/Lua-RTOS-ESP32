/*
 * Lua RTOS, 2Y0A21 sensor (proximity)
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
 * Extracted from:
 *
 * https://github.com/jeroendoggen/Arduino-GP2Y0A21YK-library/blob/master/DistanceGP2Y0A21YK/DistanceGP2Y0A21YK.cpp
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_2Y0A21

#include "2y0a21.h"

#include <math.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

driver_error_t *s2y0a21_setup(sensor_instance_t *unit);
driver_error_t *s2y0a21_acquire(sensor_instance_t *unit, sensor_value_t *values);

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
 * Helper functions
 */

driver_error_t  *distance(sensor_instance_t *unit, sensor_value_t *values, int *d) {
	driver_error_t *error;
	int p = 0;
	int sum = 0;
	int foo = 0;
	int previous = 0;
	int raw = 0;
	double mvolts = 0;

	for (int i = 0; i < 25; i++) {
		// Read value
		if ((error = adc_read(&unit->setup.adc.h, &raw, &mvolts))) {
			return error;
		}

		foo = 27.728 * pow(mvolts / 1000.0, -1.2045);

		if (foo >= (93 * previous)) {
			previous = foo;
			sum = sum + foo;
			p++;

		}
	}

	*d = sum / p;

	return NULL;

}

/*
 * Operation functions
 */
driver_error_t *s2y0a21_setup(sensor_instance_t *unit) {
	return NULL;
}

driver_error_t *s2y0a21_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	int d;

	if ((error = distance(unit, values, &d))) {
		return error;
	}

	values->floatd.value = (float)d;

	return NULL;
}

#endif
#endif

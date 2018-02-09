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
 * Lua RTOS, t2Y0A21 sensor (proximity)
 *
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

#include <math.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

driver_error_t *s2y0a21_setup(sensor_instance_t *unit);
driver_error_t *s2y0a21_acquire(sensor_instance_t *unit, sensor_value_t *values);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) s2y0a21_sensor = {
	.id = "2Y0A21",
	.interface = {
		{.type = ADC_INTERFACE},
	},
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
		if ((error = adc_read(&unit->setup[0].adc.h, &raw, &mvolts))) {
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

/*
 * Lua RTOS, A49E Linear Hall Effect Sensor
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
#if CONFIG_LUA_RTOS_USE_SENSOR_A49E

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

// Number of samples
#define A49E_SAMPLES 10

// Vout at B = 0 for a given Vcc
//
// This equation is extracted for the following experimental meassures using
// https://mycurvefit.com
//
// VCC  VOUT
// ---- ----
// 3000	1573
// 3100	1624
// 3200	1672
// 3300	1726
// 3400	1775
// 3500	1826
// 3600	1877
// 3700	1926
// 3800	1977
// 3900	2026
// 4000	2081
// 4100	2130
// 4200	2184
// 4300	2236
// 4400	2285
// 4500	2337
// 4600	2385
// 4700	2436
// 4800	2487
// 4900	2535
// 5000	2585
// 5100	2633
// 5200	2682
// 5300	2732
// 5400	2780
// 5500	2829
// 5600	2876
// 5700	2928
// 5800	2975
// 5900	3025
// 6000	3075
#define A49E_VOUT_B0(vcc) ((double)0.5083158 * (double)vcc + (double)47.05263)

// From datasheet we can deduce the gradient of the transfer function
#define A49E_M ((double)5 / (double)3)

// Using A49E_VOUT_B0(vcc), the current Vout and A49E_M we can find the magnetic field
// with this equation
//
// B = (Vout - A49E_VOUT_B0) / A49E_M

driver_error_t *a49e_acquire(sensor_instance_t *unit, sensor_value_t *values);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) a49e_sensor = {
	.id = "A49E",
	.interface = ADC_INTERFACE,
	.data = {
		{.id = "magnetic field", .type = SENSOR_DATA_FLOAT},
	},
	.acquire = a49e_acquire
};

/*
 * Operation functions
 */
driver_error_t *a49e_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;

	// Take some samples, and get average
	int raw;
	double avg = 0.0;
	double mvolts = 0;
	int i;

	for(i=1; i <= A49E_SAMPLES;i++) {
		// Read value
		if ((error = adc_read(&unit->setup.adc.h, &raw, &mvolts))) {
			return error;
		}

		avg = ((i - 1) * avg + mvolts) / i;
	}

	mvolts = avg;

	// Get channel info
	adc_channel_t *chan;

	adc_get_channel(&unit->setup.adc.h, &chan);

	values->floatd.value = ((mvolts - A49E_VOUT_B0(chan->pvref)) / A49E_M);

	return NULL;
}

#endif
#endif

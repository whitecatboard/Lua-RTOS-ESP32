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
 * Lua RTOS, A49E Linear Hall Effect Sensor
 *
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
	.interface = {
		{.type = ADC_INTERFACE},
	},
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
	double mvolts = 0;

	// Read average for some samples
	if ((error = adc_read_avg(&unit->setup[0].adc.h, A49E_SAMPLES, NULL, &mvolts))) {
		return error;
	}

	// Get channel info
	adc_chann_t *chan;

	adc_get_channel(&unit->setup[0].adc.h, &chan);

	values->floatd.value = ((mvolts - A49E_VOUT_B0(CONFIG_LUA_RTOS_VDD)) / A49E_M);

	return NULL;
}

#endif
#endif

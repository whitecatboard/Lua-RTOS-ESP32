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
 * Lua RTOS,LDR sensor
 */

#include "sdkconfig.h"

/*
                    vout
                     |
             FR      |     LDR
 	 vi --/\/\/\/\------/\/\/\/\---- gnd  (pull-up configuration)

                    vout
                     |
            LDR      |     FR
 	 vi --/\/\/\/\------/\/\/\/\---- gnd  (pull-down configuration)


 	 LDR sensor is basically a voltage divider with a fixed resistance FR that can be used as pull-up or
 	 pull-down.

 	 FR is a fixed resistor, typically a 10K resistor. It's easy to know LDR resistance knowing FR and vout.

 	 pull-up:

 	    LDR = (vout * FR) / (vi - vo)

 	 pull-down:

 	    LDR = (FR * (vi - vo)) / v0


	Equation extracted from https://www.youtube.com/watch?v=t4TnmNQo24Y

	  lux = (1.25 * 10^7) * R^-1.406
 */
#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_LDR

#define LDR_SAMPLES 10

#include <math.h>
#include <string.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

driver_error_t *ldr_setup(sensor_instance_t *unit);
driver_error_t *ldr_acquire(sensor_instance_t *unit, sensor_value_t *values);
driver_error_t *ldr_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) ldr_sensor = {
	.id = "LDR",
	.interface = {
		{.type = ADC_INTERFACE},
	},
	.data = {
		{.id = "illuminance", .type = SENSOR_DATA_FLOAT},
	},
	.properties = {
		{.id = "R1", .type = SENSOR_DATA_INT},
		{.id = "R2", .type = SENSOR_DATA_INT},
	},
	.setup = ldr_setup,
	.acquire = ldr_acquire,
	.set = ldr_set
};

/*
 * Operation functions
 */

driver_error_t *ldr_setup(sensor_instance_t *unit) {
	// Default as pull-up configuration with fixed 10K resistor
	unit->properties[0].integerd.value = 10000;
	unit->properties[1].integerd.value = 0;

	return NULL;
}

driver_error_t *ldr_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	double mvolts = 0;

	// Read average for some samples
	if ((error = adc_read_avg(&unit->setup[0].adc.h, LDR_SAMPLES, NULL, &mvolts))) {
		return error;
	}

	// Get channel info
	adc_chann_t *chan;

	adc_get_channel(&unit->setup[0].adc.h, &chan);

	// Read LDR resistance
	float ldr;

	if (unit->properties[0].integerd.value != 0) {
		// pull-up configuration
		ldr = (mvolts * unit->properties[0].integerd.value) / (CONFIG_LUA_RTOS_VDD - mvolts);
	} else {
		// pull-down configuration
		ldr = (unit->properties[1].integerd.value * (CONFIG_LUA_RTOS_VDD - mvolts)) / mvolts;
	}

	// Estimate illuminance (2 decimals)
	values[0].floatd.value = roundf(100 * (((double)12500000.0)*pow(ldr, -1.406))) / 100;

	return NULL;
}

driver_error_t *ldr_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting) {
	if (strcmp(id,"R1") == 0) {
		unit->properties[0].integerd.value = setting->integerd.value;
	}

	if (strcmp(id,"R2") == 0) {
		unit->properties[1].integerd.value = setting->integerd.value;
	}

	return NULL;
}
#endif
#endif

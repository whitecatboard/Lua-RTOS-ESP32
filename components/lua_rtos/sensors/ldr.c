/*
 * Lua RTOS, LDR sensor
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
		ADC_INTERFACE,
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
	adc_channel_t *chan;

	adc_get_channel(&unit->setup[0].adc.h, &chan);

	// Read LDR resistance
	float ldr;

	if (unit->properties[0].integerd.value != 0) {
		// pull-up configuration
		ldr = (mvolts * unit->properties[0].integerd.value) / (chan->pvref - mvolts);
	} else {
		// pull-down configuration
		ldr = (unit->properties[1].integerd.value * (chan->pvref - mvolts)) / mvolts;
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

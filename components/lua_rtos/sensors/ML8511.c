/*
 * Lua RTOS, ML8511 sensor (UV Light Intensity)
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
#if CONFIG_LUA_RTOS_USE_SENSOR_ML8511

#define ML8511_SAMPLES 10

#include <math.h>
#include <string.h>

#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>
#include <drivers/gpio.h>

driver_error_t *ML8511_presetup(sensor_instance_t *unit);
driver_error_t *ML8511_acquire(sensor_instance_t *unit, sensor_value_t *values);
driver_error_t *ML8511_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) ML8511_sensor = {
	.id = "ML8511",
	.interface = {
		{.type = ADC_INTERFACE},
	},
	.data = {
		{.id = "intensity",  .type = SENSOR_DATA_FLOAT},
	},
	.properties = {
		{.id = "calibration"  ,.type = SENSOR_DATA_FLOAT},
		{.id = "enable signal",.type = SENSOR_DATA_INT},
	},
	.presetup = ML8511_presetup,
	.acquire = ML8511_acquire,
	.set = ML8511_set
};

/*
 * Operation functions
 */

driver_error_t *ML8511_presetup(sensor_instance_t *unit) {
	unit->setup[0].adc.max = CONFIG_LUA_RTOS_VDD;

	unit->properties[0].integerd.value = 0;  // Calibration = 0
	unit->properties[1].integerd.value = -1; // No enable signal

	return NULL;
}

driver_error_t *ML8511_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	double mvolts;
	double volts;

	// Enable, if enable signal is used
	if (unit->properties[1].integerd.value >= 0) {
		error = gpio_pin_set(unit->properties[1].integerd.value);
		if (error) {
			return error;
		}

		delay(1);
	}


	// Read average for some samples
	if ((error = adc_read_avg(&unit->setup[0].adc.h, ML8511_SAMPLES, NULL, &mvolts))) {
		return error;
	}

	// Disable, if enable signal is used
	if (unit->properties[1].integerd.value >= 0) {
		error = gpio_pin_clr(unit->properties[1].integerd.value);
		if (error) {
			return error;
		}
	}

	// Convert mvolts to volts
	volts = mvolts / 1000.0;

	// Convert, 2 decimal places
	values[0].floatd.value = roundf(100 * (((volts - 2.2) / 0.129) + 10)) / 100;
	values[0].floatd.value += unit->properties[2].floatd.value;

	return NULL;
}

driver_error_t *ML8511_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting) {
	driver_error_t *error;

	if (strcmp(id,"calibration") == 0) {
		memcpy(&unit->properties[0], setting, sizeof(sensor_value_t));
	} else if (strcmp(id,"enable signal") == 0) {
		memcpy(&unit->properties[1], setting, sizeof(sensor_value_t));

		if (unit->properties[1].integerd.value >= 0) {
			error = gpio_pin_output(unit->properties[1].integerd.value);
			if (error) {
				return error;
			}

			error = gpio_pin_clr(unit->properties[1].integerd.value);
			if (error) {
				return error;
			}
		}
	}

	return NULL;
}

#endif
#endif

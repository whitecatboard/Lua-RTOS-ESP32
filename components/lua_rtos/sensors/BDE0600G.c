/*
 * Lua RTOS, BDE0600G sensor (Temperature)
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
#if CONFIG_LUA_RTOS_USE_SENSOR_BDE0600G

#define BDE0600G_SAMPLES 10

#include <math.h>
#include <string.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

driver_error_t *BDE0600G_presetup(sensor_instance_t *unit);
driver_error_t *BDE0600G_acquire(sensor_instance_t *unit, sensor_value_t *values);
driver_error_t *BDE0600G_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) BDE0600G_sensor = {
	.id = "BDE0600G",
	.interface = {
		{.type = ADC_INTERFACE},
	},
	.data = {
			{.id = "temperature", .type = SENSOR_DATA_FLOAT},
	},
	.properties = {
		{.id = "calibration",.type = SENSOR_DATA_FLOAT},
	},
	.presetup = BDE0600G_presetup,
	.acquire = BDE0600G_acquire,
	.set = BDE0600G_set
};

/*
 * Operation functions
 */

driver_error_t *BDE0600G_presetup(sensor_instance_t *unit) {
	unit->setup[0].adc.max = CONFIG_LUA_RTOS_VDD;

	unit->properties[0].integerd.value = 0; // Calibration = 0

	return NULL;
}

driver_error_t *BDE0600G_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	double mvolts;
	double volts;

	// Read average for some samples
	if ((error = adc_read_avg(&unit->setup[0].adc.h, BDE0600G_SAMPLES, NULL, &mvolts))) {
		return error;
	}

	// Convert mvolts to volts
	volts = mvolts / 1000.0;

	// Calculate and round to 2 decimals
	values[0].floatd.value = roundf(100 * (((volts-1.753)/(-0.01068)) + 30)) / 100;
	values[0].floatd.value += unit->properties[0].floatd.value;

	return NULL;
}

driver_error_t *BDE0600G_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting) {
	if (strcmp(id,"calibration") == 0) {
			memcpy(&unit->properties[0], setting, sizeof(sensor_value_t));
	}

	return NULL;
}

#endif
#endif

/*
 * Lua RTOS, BH1620FVC sensor (Ambient Light)
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
#if CONFIG_LUA_RTOS_USE_SENSOR_BH1620FVC

#define BH1620FVC_SAMPLES 10

#include <math.h>
#include <string.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

driver_error_t *BH1620FVC_presetup(sensor_instance_t *unit);
driver_error_t *BH1620FVC_acquire(sensor_instance_t *unit, sensor_value_t *values);
driver_error_t *BH1620FVC_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) BH1620FVC_sensor = {
	.id = "BH1620FVC",
	.interface = {
		{.type = ADC_INTERFACE},
	},
	.data = {
			{.id = "illuminance", .type = SENSOR_DATA_FLOAT},
	},
	.properties = {
		{.id = "gain",       .type = SENSOR_DATA_INT},
		{.id = "r1",         .type = SENSOR_DATA_INT},
		{.id = "calibration",.type = SENSOR_DATA_FLOAT},
	},
	.presetup = BH1620FVC_presetup,
	.acquire = BH1620FVC_acquire,
	.set = BH1620FVC_set
};

/*
 * Operation functions
 */

driver_error_t *BH1620FVC_presetup(sensor_instance_t *unit) {
	unit->setup[0].adc.max = CONFIG_LUA_RTOS_VDD;

	unit->properties[0].integerd.value = 0; // H-Gain Mode
	unit->properties[1].integerd.value = 5600; // 5600 ohms
	unit->properties[2].integerd.value = 0; // Calibration = 0

	return NULL;
}

driver_error_t *BH1620FVC_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	double mvolts;
	double volts;

	// Read average for some samples
	if ((error = adc_read_avg(&unit->setup[0].adc.h, BH1620FVC_SAMPLES, NULL, &mvolts))) {
		return error;
	}

	// Convert mvolts to volts
	volts = mvolts / 1000.0;

	// Convert, 2 decimal places
	if (unit->properties[0].integerd.value == 0) {
		//  H-Gain mode
		values[0].floatd.value = roundf(100 * (volts / (0.57 * 0.000001 * unit->properties[1].integerd.value))) / 100;
	} else if (unit->properties[0].integerd.value == 1) {
		//  M-Gain mode
		values[0].floatd.value = roundf(100 * (volts / (0.057 * 0.000001 * unit->properties[1].integerd.value))) / 100;
	} else if (unit->properties[0].integerd.value == 2) {
		//  L-Gain mode
		values[0].floatd.value = roundf(100 * (volts / (0.0057 * 0.000001 * unit->properties[1].integerd.value))) / 100;
	}

	values[0].floatd.value += unit->properties[2].floatd.value;

	return NULL;
}

driver_error_t *BH1620FVC_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting) {
	if (strcmp(id,"gain") == 0) {
		if ((setting->integerd.value < 0) || (setting->integerd.value > 2)) {
			return driver_error(SENSOR_DRIVER, SENSOR_ERR_INVALID_VALUE, NULL);
		}

		memcpy(&unit->properties[0], setting, sizeof(sensor_value_t));
	} else if (strcmp(id,"r1") == 0) {
		memcpy(&unit->properties[1], setting, sizeof(sensor_value_t));
	} else if (strcmp(id,"calibration") == 0) {
			memcpy(&unit->properties[2], setting, sizeof(sensor_value_t));
	}

	return NULL;
}

#endif
#endif

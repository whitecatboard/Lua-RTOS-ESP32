/*
 * Lua RTOS, Thermistor sensor (temperature)
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
 * Based on https://learn.adafruit.com/thermistor/using-a-thermistor
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_THERMISTOR

#include <math.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>

#define THERMISTORSAMPLES  5
#define THERMISTORNOMINAL  10000
#define SERIESRESISTOR     10000
#define BCOEFFICIENT       3950
#define TEMPERATURENOMINAL 25

driver_error_t *thermistor_setup(sensor_instance_t *unit);
driver_error_t *thermistor_acquire(sensor_instance_t *unit, sensor_value_t *values);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) thermistor_sensor = {
	.id = "THERMISTOR",
	.interface = {
		{.type = ADC_INTERFACE},
	},
	.data = {
		{.id = "temperature", .type = SENSOR_DATA_FLOAT},
	},
	.setup = thermistor_setup,
	.acquire = thermistor_acquire
};

/*
 * Operation functions
 */
driver_error_t *thermistor_setup(sensor_instance_t *unit) {
	return NULL;
}

driver_error_t *thermistor_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	adc_channel_t *adc;
	driver_error_t *error;

	// Get ADC channel from handle
	if ((error = adc_get_channel(&unit->setup[0].adc.h, &adc))) {
		return error;
	}

	// Take some samples, and get average
	double avg = 0.0;
	int raw = 0;
	int i;

	for(i=1; i <= THERMISTORSAMPLES;i++) {
		// Read value
		if ((error = adc_read(&unit->setup[0].adc.h, &raw, NULL))) {
			return error;
		}

		avg = ((i - 1) * avg + raw) / i;
	}

	// Convert avg to resistance
	avg = SERIESRESISTOR / (((double)(adc->max_val) / avg) - 1.0);

	// Get temperature
	double steinhart;
	steinhart = avg / THERMISTORNOMINAL;              // (R/Ro)
	steinhart = log(steinhart);                       // ln(R/Ro)
	steinhart /= BCOEFFICIENT;                        // 1/B * ln(R/Ro)
	steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
	steinhart = 1.0 / steinhart;                      // Invert
	steinhart -= 273.15;                              // convert to C

	values->floatd.value = steinhart;

	return NULL;
}

#endif
#endif

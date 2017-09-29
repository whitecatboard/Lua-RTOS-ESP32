/*
 * Lua RTOS, US-015 (Distance Sensor)
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

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_US015

#include "freertos/FreeRTOS.h"

#include <math.h>
#include <string.h>

#include <sys/delay.h>
#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/gpio.h>

driver_error_t *us015_setup(sensor_instance_t *unit);
driver_error_t *us015_acquire(sensor_instance_t *unit, sensor_value_t *values);
driver_error_t *us015_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) us015_sensor = {
	.id = "US015",
	.interface = {
		{.type = GPIO_INTERFACE},
		{.type = GPIO_INTERFACE},
	},
	.data = {
		{.id = "distance", .type = SENSOR_DATA_DOUBLE},
	},
	.interface_name = {"TRIG", "ECHO"},
	.properties = {
		{.id = "calibration", .type = SENSOR_DATA_DOUBLE},
		{.id = "temperature", .type = SENSOR_DATA_DOUBLE},
	},
	.setup = us015_setup,
	.acquire = us015_acquire,
	.set = us015_set
};

/*
 * Operation functions
 */
driver_error_t *us015_setup(sensor_instance_t *unit) {
	// Set default calibration value
	unit->properties[0].doubled.value = 0;

	// Set temperature to 20 ºC if no current temperature is provided
	unit->properties[1].doubled.value = 20;

	// Configure TRIG pin as output
	gpio_pin_output(unit->setup[0].gpio.gpio);

	gpio_pin_clr(unit->setup[0].gpio.gpio);
	udelay(200);

	// Configure ECHO pin as input
	gpio_pin_input(unit->setup[1].gpio.gpio);

	// Ignore some measures
	sensor_value_t tmp[2];
	int i;

	for(i = 0;i < 4;i++) {
		us015_acquire(unit, tmp);
		udelay(200);
	}

	return NULL;
}

driver_error_t *us015_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting) {
	if (strcmp(id,"calibration") == 0) {
		memcpy(&unit->properties[0], setting, sizeof(sensor_value_t));
	} else if (strcmp(id,"temperature") == 0) {
		memcpy(&unit->properties[1], setting, sizeof(sensor_value_t));
	}

	return NULL;
}

driver_error_t *us015_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	// Trigger pulse
	gpio_pin_set(unit->setup[0].gpio.gpio);
	udelay(10);
	gpio_pin_clr(unit->setup[0].gpio.gpio);

	// Get echo pulse width in usecs
	double time = gpio_get_pulse_time(unit->setup[1].gpio.gpio, 1, 18500);
	if (time < 0.0) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_TIMEOUT, NULL);
	}

	/*
	 * Calculate distance
	 *
	 * Sound speed depends on ambient temperature
	 * sound speed = 331.5 + (0.6 * temperature) m/sec
	 * sound speed = 331500 + (60 * temperature) mm/sec
	 *
	 */

	// First calculate mm per usec
	double mm_per_usecs = (331500.0 + (60.0 * (unit->properties[1].doubled.value))) / 1000000.0;

	// Calculate distance in centimeters.
	// Please, take note that the width of the echo is the time that the ultrasonic pulse takes to travel
	// from the sensor to the object, plus the time to back to the sensor, so we have to consider time / 2.
	// 1 decimal precision.
	if (time == 0) {
		values[0].doubled.value = 2;
	} else {
		values[0].doubled.value =
			  round((((((time / 2.0)) * mm_per_usecs) / 10.0 +
			  unit->properties[0].doubled.value)) * 10.00) / 10.00;
	}

	// Next value can get in 200 useconds
	gettimeofday(&unit->next, NULL);
	unit->next.tv_usec += 200;

	return NULL;
}

#endif
#endif

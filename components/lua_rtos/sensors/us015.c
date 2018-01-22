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
 * Lua RTOS, US-015 (Distance Sensor)
 *
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

/*
 * Lua RTOS, Analog joystick
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
#if CONFIG_LUA_RTOS_USE_SENSOR_ANALOG_JOYSTICK

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <math.h>
#include <string.h>
#include <unistd.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>
#include <drivers/gpio_debouncing.h>

driver_error_t *analog_joystick_setup(sensor_instance_t *unit);
driver_error_t *analog_joystick_acquire(sensor_instance_t *unit, sensor_value_t *values);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) analog_joystick_sensor = {
	.id = "ANALOG_JOYSTICK",
	.interface = {
		{.type = ADC_INTERFACE, .flags = SENSOR_FLAG_AUTO_ACQ}, // X
		{.type = ADC_INTERFACE, .flags = SENSOR_FLAG_AUTO_ACQ}, // Y

		// 1000 usecs debouncing threshold period, update sw property (#2)
		{
			.type = GPIO_INTERFACE, // Switch

			.flags = SENSOR_FLAG_ON_OFF | SENSOR_FLAG_ON_L | SENSOR_FLAG_DEBOUNCING | (1000 << 16) | (2 << 8)
		}
	},
	.interface_name = {"X", "Y", "SW"},
	.data = {
		{.id = "x",  .type = SENSOR_DATA_INT},
		{.id = "y",  .type = SENSOR_DATA_INT},
		{.id = "sw", .type = SENSOR_DATA_INT},
	},
	.setup = analog_joystick_setup,
	.acquire = analog_joystick_acquire,
};

/*
 * Helper functions
 */

static void sensor_task(void *arg) {
	sensor_instance_t *unit = (sensor_instance_t *)arg;
	double mvoltsx, mvoltsy;
	uint8_t first = 1;

	// Get channel info
	adc_channel_t *chanx;
	adc_channel_t *chany;

	adc_get_channel(&unit->setup[0].adc.h, &chanx);
	adc_get_channel(&unit->setup[1].adc.h, &chany);

    for(;;) {
    	// Get current voltage for x / y
    	adc_read(&unit->setup[0].adc.h, NULL, &mvoltsx);
    	adc_read(&unit->setup[1].adc.h, NULL, &mvoltsy);

    	// x signal is ADC from 0 to VRef+. The mid point is approximately Vref+ / 2
    	// y signal is ADC from 0 to VRef+. The mid point is approximately Vref+ / 2
    	//
    	// we treat x / y signal as a potentiometer value from -1.0 (-100 %) to 1.0 (100 %)
    	double cx = roundf(100 * (mvoltsx - ((double)chanx->pvref / (double)2.0)) / ((double)chanx->pvref / (double)2.0)) / (double)100;
    	double cy = roundf(100 * (mvoltsy - ((double)chany->pvref / (double)2.0)) / ((double)chany->pvref / (double)2.0)) / (double)100;

    	int8_t x = 0;
    	int8_t y = 0;

    	if (cx >= 0.5) {
    		x = 1;
    	} else if (cx <= -0.5) {
    		x = -1;
    	}

    	if (cy >= 0.5) {
    		y = 1;
    	} else if (cy <= -0.5) {
    		y = -1;
    	}

    	if (first) {
    		unit->data[0].integerd.value = x;
    		unit->data[1].integerd.value = y;

    		unit->latch[0].integerd.value = x;
    		unit->latch[1].integerd.value = y;

        	first = 0;
    	} else {
    		unit->latch[0].integerd.value = unit->data[0].integerd.value;
			unit->data[0].integerd.value = x;

    		unit->latch[1].integerd.value = unit->data[1].integerd.value;
			unit->data[1].integerd.value = y;

			sensor_queue_callbacks(unit);
    	}

    	usleep(1000);
    }
}

/*
 * Operation functions
 */

driver_error_t *analog_joystick_setup(sensor_instance_t *unit) {
	xTaskCreatePinnedToCore(sensor_task, "joystick", 1024, (void *)(unit), 21, NULL, 0);

	return NULL;
}

driver_error_t *analog_joystick_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	return NULL;

	driver_error_t *error;
	double mvolts = 0;

	if ((error = adc_read(&unit->setup[0].adc.h, NULL, &mvolts))) {
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
#endif
#endif

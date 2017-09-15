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

#define ANALOG_JOYSTICK_REPEAT 500

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

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) analog_joystick_sensor = {
	.id = "ANALOG_JOYSTICK",
	.interface = {
		{.type = ADC_INTERFACE, .flags = SENSOR_FLAG_AUTO_ACQ}, // X
		{.type = ADC_INTERFACE, .flags = SENSOR_FLAG_AUTO_ACQ}, // Y

		// 1000 usecs debouncing threshold period, update sw property (#2)
		{
			.type = GPIO_INTERFACE, // Switch

			.flags = SENSOR_FLAG_ON_OFF | SENSOR_FLAG_ON_L | SENSOR_FLAG_DEBOUNCING | (10000 << 16) | (2 << 8)
		}
	},
	.interface_name = {"X", "Y", "SW"},
	.data = {
		{.id = "x",  .type = SENSOR_DATA_INT},
		{.id = "y",  .type = SENSOR_DATA_INT},
		{.id = "sw", .type = SENSOR_DATA_INT},
	},
	.setup = analog_joystick_setup
};

/*
 * Helper functions
 */

static void sensor_task(void *arg) {
	sensor_instance_t *unit = (sensor_instance_t *)arg;
	double mvoltsx, mvoltsy;
	uint8_t first = 1;
	struct timeval now;
	uint64_t t0, t1, t;

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
    	double cx = roundf(100 * (mvoltsx - ((double)CONFIG_LUA_RTOS_VDD / (double)2.0)) / ((double)CONFIG_LUA_RTOS_VDD / (double)2.0)) / (double)100;
    	double cy = roundf(100 * (mvoltsy - ((double)CONFIG_LUA_RTOS_VDD / (double)2.0)) / ((double)CONFIG_LUA_RTOS_VDD / (double)2.0)) / (double)100;

    	int8_t x = 0;
    	int8_t y = 0;

    	// if >= 0.5 left, if <= -0.5 right
    	if (cx >= 0.5) {
    		x = 1;
    	} else if (cx <= -0.5) {
    		x = -1;
    	}

    	// if >= 0.5 down, if <= -0.5 up
    	if (cy >= 0.5) {
    		y = 1;
    	} else if (cy <= -0.5) {
    		y = -1;
    	}

    	if (first) {
    		// First data, simply store
    		gettimeofday(&now, NULL);

    		unit->data[0].integerd.value = x;
    		unit->data[1].integerd.value = y;

    		unit->latch[0].timeout = 0;
    		unit->latch[0].t = now;

    		unit->latch[1].timeout = 0;
    		unit->latch[1].t = now;

    		unit->latch[0].value.integerd.value = x;
    		unit->latch[1].value.integerd.value = y;

        	first = 0;
    	} else {
    		// Get current time in usecs
    		gettimeofday(&now, NULL);
    		t = now.tv_sec * 1000000 + now.tv_usec;

    		// Latch / store data
    		unit->latch[0].value.integerd.value = unit->data[0].integerd.value;
			unit->data[0].integerd.value = x;

    		unit->latch[1].value.integerd.value = unit->data[1].integerd.value;
			unit->data[1].integerd.value = y;

			// If x changed update latch times
			if ((unit->latch[0].value.integerd.value !=  unit->data[0].integerd.value)) {
				unit->latch[0].timeout = 0;
				unit->latch[0].t = now;
			}

			// If y changed update latch times
			if ((unit->latch[1].value.integerd.value !=  unit->data[1].integerd.value)) {
				unit->latch[1].timeout = 0;
				unit->latch[1].t = now;
			}

			// Get last latch time in usecs
			t0 = unit->latch[0].t.tv_sec * 1000000 + unit->latch[0].t.tv_usec;
			t1 = unit->latch[1].t.tv_sec * 1000000 + unit->latch[1].t.tv_usec;

			// Test if x latch is timeout
			if ((t - t0 >= 1000 * ANALOG_JOYSTICK_REPEAT) && (unit->data[0].integerd.value != 0)) {
				unit->latch[0].timeout = 1;
				unit->latch[0].t = now;
			}

			// Test if y latch is timeout
			if ((t - t1 >= 1000 * ANALOG_JOYSTICK_REPEAT) && (unit->data[1].integerd.value != 0)) {
				unit->latch[1].timeout = 1;
				unit->latch[1].t = now;
			}

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

#endif
#endif

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
 * Lua RTOS, Analog joystick
 *
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
			.flags = SENSOR_FLAG_ON_OFF |
					 SENSOR_FLAG_ON_H(0) | SENSOR_FLAG_ON_L(1) |
					 SENSOR_FLAG_DEBOUNCING | SENSOR_FLAG_DEBOUNCING_THRESHOLD(10000) |
					 SENSOR_FLAG_PROPERTY(2)
		}
	},
	.interface_name = {"X", "Y", "SW"},
	.data = {
		{.id = "x",  .type = SENSOR_DATA_INT},
		{.id = "y",  .type = SENSOR_DATA_INT},
		{.id = "sw", .type = SENSOR_DATA_INT},
	},
	.properties = {
		{.id = "repeat",       .type = SENSOR_DATA_INT},
		{.id = "repeat delay", .type = SENSOR_DATA_INT},
		{.id = "repeat rate",  .type = SENSOR_DATA_INT},
	},
	.setup = analog_joystick_setup
};

/*
 * Helper functions
 */

static void sensor_task(void *arg) {
	sensor_instance_t *unit = (sensor_instance_t *)arg;
	double mvoltsx, mvoltsy;
	sensor_value_t *data;

    data = calloc(1,sizeof(sensor_deferred_data_t) * SENSOR_MAX_PROPERTIES);
    assert(data);

	sensor_init_data(unit);

	// Get channel info
	adc_chann_t *chanx;
	adc_chann_t *chany;

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

    	// Compute sensor data (x / y movement)
    	data[0].integerd.value = x;
    	data[1].integerd.value = y;

    	// Update data
    	// Ignore center position for repeat
		sensor_update_data(
    		unit, 0, 1, data,
			unit->properties[0].integerd.value?unit->properties[1].integerd.value * 1000:0,
			unit->properties[0].integerd.value?unit->properties[2].integerd.value * 1000:0,
			1, 0
    	);

    	usleep(10000);
    }
}

/*
 * Operation functions
 */

driver_error_t *analog_joystick_setup(sensor_instance_t *unit) {
	// Set repeat feature by default
	// Set repeat delay to 1500 msecs
	// Set repeat rate to 500 msecs
	unit->properties[0].integerd.value = 1;
	unit->properties[1].integerd.value = 1500;
	unit->properties[2].integerd.value = 500;

	xTaskCreatePinnedToCore(sensor_task, "joystick", 1024, (void *)(unit), 21, NULL, 0);

	return NULL;
}

#endif
#endif

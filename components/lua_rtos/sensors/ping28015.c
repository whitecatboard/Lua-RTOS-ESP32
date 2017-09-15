/*
 * Lua RTOS, PING))) #28015 sensor (Distance Sensor)
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
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
#if CONFIG_LUA_RTOS_USE_SENSOR_PING28015

#include "freertos/FreeRTOS.h"

#include <math.h>
#include <string.h>

#include <sys/delay.h>
#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/gpio.h>

typedef struct {
	xQueueHandle queue;
} ping28015_args_t;

typedef struct {
	unsigned time;
} ping28015_data_t;

driver_error_t *ping28015_setup(sensor_instance_t *unit);
driver_error_t *ping28015_acquire(sensor_instance_t *unit, sensor_value_t *values);
driver_error_t *ping28015_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) ping28015_sensor = {
	.id = "PING28015",
	.interface = {
		{.type = GPIO_INTERFACE},
	},
	.data = {
		{.id = "distance", .type = SENSOR_DATA_FLOAT},
	},
	.properties = {
		{.id = "temperature", .type = SENSOR_DATA_FLOAT},
	},
	.setup = ping28015_setup,
	.acquire = ping28015_acquire,
	.set = ping28015_set
};

/*
 * Helper functions
 */
static void IRAM_ATTR isr(void* arg) {
    portBASE_TYPE high_priority_task_awoken = 0;
	ping28015_args_t *args = (ping28015_args_t *)arg;
	ping28015_data_t data;

	data.time = xthal_get_ccount();
	xQueueSendFromISR(args->queue, &data, &high_priority_task_awoken);
    if (high_priority_task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

/*
 * Operation functions
 */
driver_error_t *ping28015_setup(sensor_instance_t *unit) {
 	// Set temperature to 20 ºC if any temperature is provided
	unit->properties[0].floatd.value = 20;

	// Ignore first measure
	gpio_pin_clr(unit->setup[0].gpio.gpio);
	delay(20);

	return NULL;
}

driver_error_t *ping28015_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting) {
	if (strcmp(id,"temperature") == 0) {
		memcpy(&unit->properties[0], setting, sizeof(sensor_value_t));
	}

	return NULL;
}

driver_error_t *ping28015_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	ping28015_args_t args;
	ping28015_data_t data1, data2;

	int gpio = unit->setup[0].gpio.gpio;

	args.queue = xQueueCreate(2, sizeof(ping28015_data_t));
	if (!args.queue) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Configure pin as output and trigger pulse
	gpio_pin_output(gpio);
	gpio_pin_set(gpio);
	udelay(5);
	gpio_pin_clr(gpio);

	// Configure pin as input
	gpio_pin_input(gpio);

	error = gpio_isr_attach(gpio, isr, GPIO_INTR_ANYEDGE, (void *)&args);
	if (error) {
		vQueueDelete(args.queue);
		return error;
	}

	if (xQueueReceive(args.queue, &data1, 185 / portTICK_PERIOD_MS) == pdFALSE) {
		gpio_isr_detach(gpio);
		vQueueDelete(args.queue);
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_TIMEOUT, NULL);
	}

	if (xQueueReceive(args.queue, &data2, 185 / portTICK_PERIOD_MS) == pdFALSE) {
		gpio_isr_detach(gpio);
		vQueueDelete(args.queue);
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_TIMEOUT, NULL);
	}

	gpio_isr_detach(gpio);
	vQueueDelete(args.queue);

	unsigned start = data1.time;
	unsigned end = data2.time;

	int time = round((((double)((double)end - (double)start) / (double)(CPU_HZ / (1000000.0 * (CPU_HZ / CORE_TIMER_HZ))))) / (double)2.0);

	if (time > 185000) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_TIMEOUT, NULL);
	}

	/*
	 * Calculate distance
	 *
	 * Sound speed depends on ambient temperature
	 * sound speed = 331.5 + (0.6 * temperature) m/s
	 * sound speed = 33150 + (0.6 * temperature) cm/s
	 */

	// First calculate centimeters per usec
	double cm_per_usecs = ((33150.0 + (0.6 * ((double)unit->properties[0].floatd.value))) / 1000000.0);

	values[0].floatd.value = (float)((double)time * cm_per_usecs);

	// Delay for next measure
	udelay(200);

	return NULL;
}

#endif
#endif

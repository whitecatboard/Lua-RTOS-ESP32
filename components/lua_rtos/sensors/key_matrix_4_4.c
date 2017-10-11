/*
 * Lua RTOS, 4 x 4 key matrix sensor
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
#if CONFIG_LUA_RTOS_USE_SENSOR_KEY_MATRIX_4_4

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"

#include <string.h>
#include <unistd.h>

#include <sys/mutex.h>
#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/sensor.h>
#include <drivers/gpio.h>

driver_error_t *key_matrix_4_4_setup(sensor_instance_t *unit);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) key_matrix_4_4_sensor = {
	.id = "KEY_MATRIX_4_4",
	.interface = {
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
	},
	.interface_name = {"L1", "L2", "L3", "L4", "H1", "H2", "H3", "H4"},
	.data = {
		{.id = "key1",  .type = SENSOR_DATA_INT},
		{.id = "key2",  .type = SENSOR_DATA_INT},
		{.id = "key3",  .type = SENSOR_DATA_INT},
		{.id = "key4",  .type = SENSOR_DATA_INT},
		{.id = "key5",  .type = SENSOR_DATA_INT},
		{.id = "key6",  .type = SENSOR_DATA_INT},
		{.id = "key7",  .type = SENSOR_DATA_INT},
		{.id = "key8",  .type = SENSOR_DATA_INT},
		{.id = "key9",  .type = SENSOR_DATA_INT},
		{.id = "key10", .type = SENSOR_DATA_INT},
		{.id = "key11", .type = SENSOR_DATA_INT},
		{.id = "key12", .type = SENSOR_DATA_INT},
		{.id = "key13", .type = SENSOR_DATA_INT},
		{.id = "key14", .type = SENSOR_DATA_INT},
		{.id = "key15", .type = SENSOR_DATA_INT},
		{.id = "key16", .type = SENSOR_DATA_INT},
	},
	.properties = {
		{.id = "repeat",       .type = SENSOR_DATA_INT},
		{.id = "repeat delay", .type = SENSOR_DATA_INT},
		{.id = "repeat rate",  .type = SENSOR_DATA_INT},
	},
	.setup = key_matrix_4_4_setup,
};

/*
 * Helper functions
 */
static void key_matrix_4_4_task(void *arg) {
	driver_error_t *error;

	sensor_instance_t *unit = (sensor_instance_t *)arg;
	uint8_t i, j;
	uint8_t key;
	uint8_t press;
	uint32_t status;

	sensor_init_data(unit);

    for(;;) {
    	status = 0;

    	for(i=0;i < 4;i++) {
    		error = gpio_ll_pin_clr(unit->setup[i].gpio.gpio);
    		if (error) {
    			free(error);
    			continue;
    		}
    		delay(1);
    		for(j=0;j < 4;j++) {
    			key = j * 4 + i;
    			press = !gpio_ll_pin_get(unit->setup[j + 4].gpio.gpio);
    			status |= (1 << key) & (press  << key);
        	}
    		error = gpio_ll_pin_set(unit->setup[i].gpio.gpio);
    		if (error) {
    			free(error);
    			continue;
    		}
    		delay(1);
    	}

    	// Compute sensor data (status for each key)
    	sensor_value_t data[SENSOR_MAX_PROPERTIES];

    	for(i=0; i < 16; i++) {
    		data[i].integerd.value = (status & (1 << i)) != 0;
    	}

    	// Update data
    	sensor_update_data(
    		unit, 0, 15, data,
			unit->properties[0].integerd.value?unit->properties[1].integerd.value * 1000:0,
			unit->properties[0].integerd.value?unit->properties[2].integerd.value * 1000:0,
			0, 0
    	);

    	usleep(10000);
    }
}

/*
 * Operation functions
 */
driver_error_t *key_matrix_4_4_setup(sensor_instance_t *unit) {
	driver_unit_lock_error_t *lock_error = NULL;
	int i;

	// Lock all pins
	for(i=0;i < 8;i++) {
	    if ((lock_error = driver_lock(SENSOR_DRIVER, unit->unit, GPIO_DRIVER, unit->setup[i].gpio.gpio, DRIVER_ALL_FLAGS, unit->sensor->id))) {
	    	driver_unlock_all(SENSOR_DRIVER, unit->unit);
	    	return driver_lock_error(SENSOR_DRIVER, lock_error);
	    }
	}

	// Configure column pins as output
	for(i=0;i < 4;i++) {
		gpio_pin_output(unit->setup[i].gpio.gpio);
	}

	// Configure row pins as input / pulled up
	for(i=4;i < 8;i++) {
		gpio_pin_input(unit->setup[i].gpio.gpio);
		gpio_pin_pullup(unit->setup[i].gpio.gpio);
	}

	// Set repeat feature by default
	// Set repeat delay to 1500 msecs
	// Set repeat rate to 500 msecs
	unit->properties[0].integerd.value = 1;
	unit->properties[1].integerd.value = 1500;
	unit->properties[2].integerd.value = 500;

	// Create a task for scan the keyboard
	TaskHandle_t task;
	BaseType_t xReturn = xTaskCreatePinnedToCore(key_matrix_4_4_task, "keymatrix", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, unit, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &task, xPortGetCoreID());
	if (xReturn != pdPASS) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	return NULL;
}

#endif
#endif

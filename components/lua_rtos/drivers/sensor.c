/*
 * Lua RTOS, sensor driver
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

#if USE_SENSORS

#include <string.h>

#include <sys/list.h>
#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>
#include <drivers/gpio.h>
#include <drivers/owire.h>
#include <drivers/i2c.h>
#include <drivers/power_bus.h>

// This variable is defined at linker time
extern const sensor_t sensors[];

// Driver message errors
DRIVER_REGISTER_ERROR(SENSOR, sensor, CannotSetup, "can't setup", SENSOR_ERR_CANT_INIT);
DRIVER_REGISTER_ERROR(SENSOR, sensor, Timeout, "timeout", SENSOR_ERR_TIMEOUT);
DRIVER_REGISTER_ERROR(SENSOR, sensor, NotEnoughtMemory, "not enough memory", SENSOR_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_ERROR(SENSOR, sensor, SetupUndefined, "setup function is not defined", SENSOR_ERR_SETUP_UNDEFINED);
DRIVER_REGISTER_ERROR(SENSOR, sensor, AcquireUndefined, "acquire function is not defined", SENSOR_ERR_ACQUIRE_UNDEFINED);
DRIVER_REGISTER_ERROR(SENSOR, sensor, SetUndefined, "set function is not defined", SENSOR_ERR_SET_UNDEFINED);
DRIVER_REGISTER_ERROR(SENSOR, sensor, NotFound, "not found", SENSOR_ERR_NOT_FOUND);
DRIVER_REGISTER_ERROR(SENSOR, sensor, InterfaceNotSupported, "interface not supported", SENSOR_ERR_INTERFACE_NOT_SUPPORTED);

// List of instantiated sensors
struct list sensor_list;

/*
 * Helper functions
 */
#if USE_ADC
static driver_error_t *sensor_adc_setup(sensor_instance_t *unit) {
	driver_unit_lock_error_t *lock_error = NULL;
	driver_error_t *error;

	// Lock ADC channel
    if ((lock_error = driver_lock(SENSOR_DRIVER, unit->unit, ADC_DRIVER, unit->setup.adc.channel))) {
    	// Revoked lock on ADC channel
    	return driver_lock_error(SENSOR_DRIVER, lock_error);
    }

	if ((error = adc_setup(unit->setup.adc.unit, unit->setup.adc.channel, 0, unit->setup.adc.resolution))) {
		return error;
	}

	return NULL;
}
#endif

static driver_error_t *sensor_gpio_setup(sensor_instance_t *unit) {
	driver_unit_lock_error_t *lock_error = NULL;

	// Lock gpio
    if ((lock_error = driver_lock(SENSOR_DRIVER, unit->unit, GPIO_DRIVER, unit->setup.gpio.gpio))) {
    	// Revoked lock on gpio
    	return driver_lock_error(SENSOR_DRIVER, lock_error);
    }

    gpio_pin_output(unit->setup.gpio.gpio);
	gpio_pin_set(unit->setup.gpio.gpio);

	return NULL;
}

#if USE_OWIRE
static driver_error_t *sensor_owire_setup(sensor_instance_t *unit) {
	driver_error_t *error;

	// Check if owire interface is setup on the given gpio
	int dev = owire_checkpin(unit->setup.owire.gpio);
	if (dev < 0) {
		// setup new owire interface on given pin
		if ((error = owire_setup_pin(unit->setup.owire.gpio))) {
		  	return error;
		}
		int dev = owire_checkpin(unit->setup.owire.gpio);
		if (dev < 0) {
			return driver_setup_error(SENSOR_DRIVER, SENSOR_ERR_CANT_INIT, NULL);
		}
		vTaskDelay(10 / portTICK_RATE_MS);
		owdevice_input(dev);
		ow_devices_init(dev);
		unit->setup.owire.owdevice = dev;

		// Search for devices on owire bus
		TM_OneWire_Dosearch(dev);
	}
	else {
		unit->setup.owire.owdevice = dev;
		TM_OneWire_Dosearch(dev);
	}

	// check if owire bus is setup
	if (ow_devices[unit->setup.owire.owdevice].device.pin == 0) {
		return driver_setup_error(SENSOR_DRIVER, SENSOR_ERR_CANT_INIT, NULL);
	}

	return NULL;
}
#endif

#if USE_I2C
static driver_error_t *sensor_i2c_setup(sensor_instance_t *unit) {
	driver_error_t *error;

    if ((error = i2c_setup(unit->setup.i2c.id, I2C_MASTER, unit->setup.i2c.speed, unit->setup.i2c.sda, unit->setup.i2c.scl, 0, 0))) {
    	return error;
    }
	return NULL;
}
#endif

/*
 * Operation functions
 */
void sensor_init() {
	// Init sensor list
    list_init(&sensor_list, 0);
}

const sensor_t *get_sensor(const char *id) {
	const sensor_t *csensor;

	csensor = sensors;
	while (csensor->id) {
		if (strcmp(csensor->id,id) == 0) {
			return csensor;
		}
		csensor++;
	}

	return NULL;
}

const sensor_data_t *sensor_get_property(const sensor_t *sensor, const char *property) {
	int idx = 0;

	for(idx=0;idx <  SENSOR_MAX_PROPERTIES;idx++) {
		if (sensor->properties[idx].id) {
			if (strcmp(sensor->properties[idx].id,property) == 0) {
				return &(sensor->properties[idx]);
			}
		}
	}

	return NULL;
}

driver_error_t *sensor_setup(const sensor_t *sensor, sensor_setup_t *setup, sensor_instance_t **unit) {
	driver_error_t *error = NULL;
	sensor_instance_t *instance = NULL;
	int i = 0;

	// Sanity checks
	if (!sensor->acquire) {
		return driver_setup_error(SENSOR_DRIVER, SENSOR_ERR_ACQUIRE_UNDEFINED, NULL);
	}

	// Create a sensor instance
	if (!(instance = (sensor_instance_t *)calloc(1, sizeof(sensor_instance_t)))) {
		return driver_setup_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Store reference to sensor into instance
	instance->sensor = sensor;

	// Copy sensor setup configuration into instance
	memcpy(&instance->setup, setup, sizeof(sensor_setup_t));

	// Initialize sensor data from sensor definition into instance
	for(i=0;i<SENSOR_MAX_DATA;i++) {
		instance->data[i].type = sensor->data[i].type;
	}

	// Initialize sensor properties from sensor definition into instance
	for(i=0;i<SENSOR_MAX_PROPERTIES;i++) {
		instance->properties[i].type = sensor->properties[i].type;
	}

	// Add instance to sensor_list
	if (list_add(&sensor_list, instance, &instance->unit)) {
		free(instance);

		return driver_setup_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Setup sensor interface
	switch (sensor->interface) {
#if USE_ADC
		case ADC_INTERFACE: error = sensor_adc_setup(instance);break;
#endif
		case GPIO_INTERFACE: error = sensor_gpio_setup(instance);break;
#if USE_OWIRE
		case OWIRE_INTERFACE: error = sensor_owire_setup(instance);break;
#endif
#if USE_I2C
		case I2C_INTERFACE: error = sensor_i2c_setup(instance);break;
#endif
		default:
			return driver_setup_error(SENSOR_DRIVER, SENSOR_ERR_INTERFACE_NOT_SUPPORTED, NULL);
			break;
	}

	if (error) {
		// Remove instance
		list_remove(&sensor_list, instance->unit, 1);

		return error;
	}

	// Call to specific setup function, if any
	if (instance->sensor->setup) {
		if ((error = instance->sensor->setup(instance))) {
			// Remove instance
			list_remove(&sensor_list, instance->unit, 1);

			return error;
		}
	}

	*unit = instance;

	return NULL;
}

driver_error_t *sensor_acquire(sensor_instance_t *unit) {
	driver_error_t *error = NULL;
	sensor_value_t *value = NULL;
	int i = 0;

	#if CONFIG_LUA_RTOS_USE_POWER_BUS
	pwbus_on();
	#endif

// Allocate space for sensor data
	if (!(value = calloc(1, sizeof(sensor_value_t) * SENSOR_MAX_DATA))) {
		return driver_operation_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Call to specific acquire function, if any
	if ((error = unit->sensor->acquire(unit, value))) {
		free(value);
		return error;
	}

	// Copy sensor values into instance
	// Note that we only copy raw values as value types are set in sensor_setup from sensor
	// definition
	for(i=0;i < SENSOR_MAX_DATA;i++) {
		unit->data[i].raw = value[i].raw;
	}

	free(value);

	return NULL;
}

driver_error_t *sensor_read(sensor_instance_t *unit, const char *id, sensor_value_t **value) {
	int idx = 0;

	*value = NULL;

	for(idx=0;idx <  SENSOR_MAX_DATA;idx++) {
		if (unit->sensor->data[idx].id) {
			if (strcmp(unit->sensor->data[idx].id,id) == 0) {
				*value = &unit->data[idx];
				return NULL;
			}
		}
	}

	return driver_operation_error(SENSOR_DRIVER, SENSOR_ERR_NOT_FOUND, NULL);
}

driver_error_t *sensor_set(sensor_instance_t *unit, const char *id, sensor_value_t *value) {
	int idx = 0;

	// Sanity checks
	if (!unit->sensor->set) {
		return driver_operation_error(SENSOR_DRIVER, SENSOR_ERR_SET_UNDEFINED, NULL);
	}

	for(idx=0;idx < SENSOR_MAX_PROPERTIES;idx++) {
		if (unit->sensor->properties[idx].id) {
			if (strcmp(unit->sensor->properties[idx].id,id) == 0) {
				unit->sensor->set(unit, id, value);
				return NULL;
			}
		}
	}

	return driver_operation_error(SENSOR_DRIVER, SENSOR_ERR_NOT_FOUND, NULL);
}

driver_error_t *sensor_get(sensor_instance_t *unit, const char *id, sensor_value_t **value) {
	int idx = 0;

	*value = NULL;

	// Sanity checks
	if (!unit->sensor->get) {
		return driver_operation_error(SENSOR_DRIVER, SENSOR_ERR_SET_UNDEFINED, NULL);
	}

	for(idx=0;idx < SENSOR_MAX_PROPERTIES;idx++) {
		if (unit->sensor->properties[idx].id) {
			if (strcmp(unit->sensor->properties[idx].id,id) == 0) {
				unit->sensor->get(unit, id, &unit->properties[idx]);

				*value = &unit->properties[idx];

				return NULL;
			}
		}
	}

	return driver_operation_error(SENSOR_DRIVER, SENSOR_ERR_NOT_FOUND, NULL);
}

DRIVER_REGISTER(SENSOR,sensor,NULL,sensor_init,NULL);

#endif

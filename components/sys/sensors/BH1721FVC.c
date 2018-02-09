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
 * Lua RTOS, BH1721FVC sensor (Ambient Light)
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_BH1721FVC

#define BH1721FVC_I2C_ADDRESS 0b0100011

#include <math.h>
#include <string.h>

#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/sensor.h>
#include <drivers/i2c.h>

driver_error_t *BH1721FVC_presetup(sensor_instance_t *unit);
driver_error_t *BH1721FVC_acquire(sensor_instance_t *unit, sensor_value_t *values);
driver_error_t *BH1721FVC_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) BH1721FVC_sensor = {
	.id = "BH1721FVC",
	.interface = {
		{.type = I2C_INTERFACE},
	},
	.data = {
			{.id = "illuminance", .type = SENSOR_DATA_FLOAT},
	},
	.properties = {
		{.id = "resolution", .type = SENSOR_DATA_INT},
		{.id = "calibration",.type = SENSOR_DATA_FLOAT},
	},
	.presetup = BH1721FVC_presetup,
	.acquire = BH1721FVC_acquire,
	.set = BH1721FVC_set
};

/*
 * Operation functions
 */

driver_error_t *BH1721FVC_presetup(sensor_instance_t *unit) {
	// Set default values, if not provided
	if (unit->setup[0].i2c.devid == 0) {
		unit->setup[0].i2c.devid = BH1721FVC_I2C_ADDRESS;
	}

	if (unit->setup[0].i2c.speed == 0) {
		unit->setup[0].i2c.speed = 400000;
	}

	unit->properties[0].integerd.value = 1; // H-Resolution Mode
	unit->properties[1].floatd.value = 0; // Calibration = 0

	return NULL;
}

driver_error_t *BH1721FVC_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	uint8_t i2c = unit->setup[0].i2c.id;
	int16_t address = unit->setup[0].i2c.devid;
	driver_error_t *error;

	int transaction = I2C_TRANSACTION_INITIALIZER;
	uint8_t buff[2];

	// Power on
	buff[0] = 0x01;

	error = i2c_start(i2c, &transaction);if (error) return error;
	error = i2c_write_address(i2c, &transaction, address, 0);if (error) return error;
	error = i2c_write(i2c, &transaction, (char *)&buff, 1);if (error) return error;
	error = i2c_stop(i2c, &transaction);if (error) return error;

	delay(16);

	// Set resolution mode
	if (unit->properties[0].integerd.value == 0) {
		// Auto-Resolution Mode
		buff[0] = 0b00010000;
	} else if (unit->properties[0].integerd.value == 1) {
		// H-Resolution Mode
		buff[0] = 0b00010010;
	} else if (unit->properties[0].integerd.value == 2) {
		/// L-Resolution Mode
		buff[0] = 0b00010011;
	}

	error = i2c_start(i2c, &transaction);if (error) return error;
	error = i2c_write_address(i2c, &transaction, address, 0);if (error) return error;
	error = i2c_write(i2c, &transaction, (char *)&buff, 1);if (error) return error;
	error = i2c_stop(i2c, &transaction);if (error) return error;

	delay(180);

	// Read
	error = i2c_start(i2c, &transaction);if (error) return error;
	error = i2c_write_address(i2c, &transaction, address, 1);if (error) return error;
	error = i2c_read(i2c, &transaction, (char *)buff, 2);if (error) return error;
	error = i2c_stop(i2c, &transaction);if (error) return error;

	// Power down
	buff[0] = 0x00;

	error = i2c_start(i2c, &transaction);if (error) return error;
	error = i2c_write_address(i2c, &transaction, address, 0);if (error) return error;
	error = i2c_write(i2c, &transaction, (char *)&buff, 1);if (error) return error;
	error = i2c_stop(i2c, &transaction);if (error) return error;

	values[0].floatd.value = ((buff[0] << 8) + buff[1]) / 1.2;
	values[0].floatd.value += unit->properties[1].floatd.value;

	return NULL;
}

driver_error_t *BH1721FVC_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting) {
	if (strcmp(id,"resolution") == 0) {
		if ((setting->integerd.value < 0) || (setting->integerd.value > 2)) {
			return driver_error(SENSOR_DRIVER, SENSOR_ERR_INVALID_VALUE, NULL);
		}

		memcpy(&unit->properties[0], setting, sizeof(sensor_value_t));
	} else if (strcmp(id,"calibration") == 0) {
		memcpy(&unit->properties[1], setting, sizeof(sensor_value_t));
	}

	return NULL;
}

#endif
#endif

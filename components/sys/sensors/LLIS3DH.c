/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, LIS3DH sensor (accelerometer)
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSORLIS3DH

#define LIS3DH_I2C_ADDRESS 0x19

#include <math.h>
#include <string.h>
#include <stdint.h>

#include <math/soft-filter.h>
#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/gpio.h>

#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <drivers/vtimer.h>
#include <drivers/i2c.h>

#define LIS3DH_RESOLUTION 65536.0

// Registers
#define LIS3DH_STATUS_REG_AUX 0x07
#define LIS3DH_OUT_ADC1_L 0x08
#define LIS3DH_OUT_ADC1_H 0x09
#define LIS3DH_OUT_ADC2_L 0x0a
#define LIS3DH_OUT_ADC2_H 0x0b
#define LIS3DH_OUT_ADC3_L 0x0c
#define LIS3DH_OUT_ADC3_H 0x0d
#define LIS3DH_WHO_AM_I 0x0f
#define LIS3DH_CTRL_REG0 0x1e
#define LIS3DH_TEMP_CFG_REG 0x1f
#define LIS3DH_CTRL_REG1 0x20
#define LIS3DH_CTRL_REG2 0x21
#define LIS3DH_CTRL_REG3 0x22
#define LIS3DH_CTRL_REG4 0x23
#define LIS3DH_CTRL_REG5 0x24
#define LIS3DH_CTRL_REG6 0x25
#define LIS3DH_REFERENCE 0x26
#define LIS3DH_STATUS_REG 0x27
#define LIS3DH_OUT_X_L 0x28
#define LIS3DH_OUT_X_H 0x29
#define LIS3DH_OUT_Y_L 0x2a
#define LIS3DH_OUT_Y_H 0x2b
#define LIS3DH_OUT_Z_L 0x2c
#define LIS3DH_OUT_Z_H 0x2d
#define LIS3DH_FIFO_CTRL_REG 0x2e
#define LIS3DH_FIFO_SRC_REG 0x2f
#define LIS3DH_INT1_CFG 0x30
#define LIS3DH_INT1_SRC 0x31
#define LIS3DH_INT1_THS 0x32
#define LIS3DH_INT1_DURATION 0x33
#define LIS3DH_INT2_CFG 0x34
#define LIS3DH_INT2_SRC 0x35
#define LIS3DH_INT2_THS 0x36
#define LIS3DH_INT2_DURATION 0x37
#define LIS3DH_CLICK_CFG 0x38
#define LIS3DH_CLICK_SRC 0x39
#define LIS3DH_CLICK_THS 0x3a
#define LIS3DH_TIME_LIMIT 0x3b
#define LIS3DH_TIME_LATENCY 0x3c
#define LIS3DH_WINDOW 0x3d
#define LIS3DH_ACT_THS 0x3e
#define LIS3DH_ACT_DUR 0x3f

#define LIS3DH_CTRL_REG1_X_EN  (1 << 0)
#define LIS3DH_CTRL_REG1_Y_EN  (1 << 1)
#define LIS3DH_CTRL_REG1_Z_EN  (1 << 2)
#define LIS3DH_CTRL_REG1_LP_EN (1 << 3)

#define LIS3DH_CTRL_REG1_ODR_MASK    (0x0f << 4)
#define LIS3DH_CTRL_REG1_ODR_PWDWN   (0x00 << 4)
#define LIS3DH_CTRL_REG1_ODR_1_HZ    (0x01 << 4)
#define LIS3DH_CTRL_REG1_ODR_10_HZ   (0x02 << 4)
#define LIS3DH_CTRL_REG1_ODR_25_HZ   (0x03 << 4)
#define LIS3DH_CTRL_REG1_ODR_50_HZ   (0x04 << 4)
#define LIS3DH_CTRL_REG1_ODR_100_HZ  (0x05 << 4)
#define LIS3DH_CTRL_REG1_ODR_200_HZ  (0x06 << 4)
#define LIS3DH_CTRL_REG1_ODR_400_HZ  (0x07 << 4)
#define LIS3DH_CTRL_REG1_ODR_1600_HZ (0x08 << 4)

#define LIS3DH_CTRL_REG4_BDU     (1 << 7)
#define LIS3DH_CTRL_REG4_BLE     (1 << 6)
#define LIS3DH_CTRL_REG4_FS_MASK (0x03 << 4)
#define LIS3DH_CTRL_REG4_FS_2    (0x00 << 4)
#define LIS3DH_CTRL_REG4_FS_4    (0x01 << 4)
#define LIS3DH_CTRL_REG4_FS_8    (0x02 << 4)
#define LIS3DH_CTRL_REG4_FS_16   (0x03 << 4)
#define LIS3DH_CTRL_REG4_HR      (1 << 3)

#define LIS3DH_CTRL_REG5_BOOT     (1 << 7)
#define LIS3DH_CTRL_REG5_FIFO_EN  (1 << 6)
#define LIS3DH_CTRL_REG5_LIR_INT1 (1 << 3)
#define LIS3DH_CTRL_REG5_D4D_INT1 (1 << 2)
#define LIS3DH_CTRL_REG5_LIR_INT2 (1 << 1)
#define LIS3DH_CTRL_REG5_D4D_INT2 (1 << 0)

driver_error_t *LIS3DH_presetup(sensor_instance_t *unit);
driver_error_t *LIS3DH_setup(sensor_instance_t *unit);
driver_error_t *LIS3DH_acquire(sensor_instance_t *unit, sensor_value_t *values);
driver_error_t *LIS3DH_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting);

static uint8_t _default_ctrl1_reg;
static uint8_t _default_ctrl4_reg;
static uint16_t _divider;
static TaskHandle_t task;

static filter_t *x_g_filter = NULL;
static filter_t *y_g_filter = NULL;
static filter_t *z_g_filter = NULL;

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) LIS3DH_sensor = {
	.id = "LIS3DH",
	.interface = {
		{.type = I2C_INTERFACE},
	},
	.data = {
			{.id = "x-g", .type = SENSOR_DATA_FLOAT},
			{.id = "y-g", .type = SENSOR_DATA_FLOAT},
			{.id = "z-g", .type = SENSOR_DATA_FLOAT},
	},
	.properties = {
		{.id = "full_scale", .type = SENSOR_DATA_INT},
	},
	.presetup = LIS3DH_presetup,
	.setup = LIS3DH_setup,
	.acquire = LIS3DH_acquire,
	.set = LIS3DH_set
};

static void IRAM_ATTR _acquire(void *args) {
	portBASE_TYPE high_priority_task_awoken = 0;

	vtmr_start(2500, _acquire, NULL);

	vTaskNotifyGiveFromISR(task, &high_priority_task_awoken);

    if (high_priority_task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void _acquire_task(void *args) {
	sensor_instance_t *unit = (sensor_instance_t *)args;
	driver_error_t *error;
	uint8_t i2c = unit->setup[0].i2c.id;
	int16_t address = unit->setup[0].i2c.devid;

	x_g_filter = filter_create(FilterButterHighPass, 4, 400.0, 5);
	y_g_filter = filter_create(FilterButterHighPass, 4, 400.0, 5);
	z_g_filter = filter_create(FilterButterHighPass, 4, 400.0, 5);

	while (1) {
		// Wait for notification
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Start measuring
		error = i2c_write_register(i2c, address, LIS3DH_CTRL_REG1, _default_ctrl1_reg | LIS3DH_CTRL_REG1_ODR_400_HZ);
		if (error) {
			free(error);
			continue;
		}

		// Wait for data available
		uint8_t status;

		while ((status & 0x04) != 0x04) {
			error = i2c_read_register(i2c, address, LIS3DH_STATUS_REG, &status);
			if (error) {
				free(error);
				continue;
			}
		}

		// Read acceleration in auto increment mode
		uint8_t buffer[6];

		error = i2c_read_register_block(i2c, address, 0x80 | LIS3DH_OUT_X_L, buffer, sizeof(buffer));
		if (error) {
			free(error);
			continue;
		}

		// Get RAW data
		int16_t x_raw;
		int16_t y_raw;
		int16_t z_raw;

		x_raw = ((buffer[1] << 8) | buffer[0]);
		y_raw = ((buffer[3] << 8) | buffer[2]);
		z_raw = ((buffer[5] << 8) | buffer[4]);

		// Get G data
		float x_g;
		float y_g;
		float z_g;

		x_g = ((float)x_raw) / ((float)_divider);
		y_g = ((float)y_raw) / ((float)_divider);
		z_g = ((float)z_raw) / ((float)_divider);

		//x_g += 0.031215;
		//y_g += 0.031215;
		//z_g += 0.031215;

		x_g = filter_value(x_g_filter, x_g);
		y_g = filter_value(y_g_filter, y_g);
		z_g = filter_value(z_g_filter, z_g);

		if (z_g >= 0.1) {
			printf("%f\r\n",z_g);
		}
	}
}

/*
 * Operation functions
 */

driver_error_t *LIS3DH_presetup(sensor_instance_t *unit) {
	// Set default values, if not provided
	if (unit->setup[0].i2c.devid == 0) {
		unit->setup[0].i2c.devid = LIS3DH_I2C_ADDRESS;
	}

	if (unit->setup[0].i2c.speed == 0) {
		unit->setup[0].i2c.speed = 400000;
	}

	unit->properties[0].integerd.value = 2; // +/- 2g full scale

	return NULL;
}

driver_error_t *LIS3DH_setup(sensor_instance_t *unit) {
	uint8_t i2c = unit->setup[0].i2c.id;
	int16_t address = unit->setup[0].i2c.devid;

	driver_error_t *error;
	uint8_t data;

	// Probe sensor
	error = i2c_read_register(i2c, address, LIS3DH_WHO_AM_I, &data);if (error) return error;
	if (data != 0x33) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_FOUND, NULL);
	}

	// This value is required for correct operation of the device
	error = i2c_write_register(i2c, address, LIS3DH_CTRL_REG0, 0x10);if (error) return error;

	// Disable ADC and temperature
	error = i2c_write_register(i2c, address, LIS3DH_TEMP_CFG_REG, 0x00);if (error) return error;

	// Configure as: high resolution / normal mode, and all axis enabled, power down mode
	_default_ctrl1_reg = LIS3DH_CTRL_REG1_X_EN |
		                 LIS3DH_CTRL_REG1_Y_EN |
		                 LIS3DH_CTRL_REG1_Z_EN;

	error = i2c_write_register(i2c, address, LIS3DH_CTRL_REG1, _default_ctrl1_reg);if (error) return error;

	// No filter used
	error = i2c_write_register(i2c, address, LIS3DH_CTRL_REG2, 0x00);if (error) return error;

	// No interrupts used
	error = i2c_write_register(i2c, address, LIS3DH_CTRL_REG3, 0x00);if (error) return error;

	// Configured as: no continuous update, LSB first, +/- 2g full scale, high resolution enabled
	_default_ctrl4_reg = LIS3DH_CTRL_REG4_BDU | LIS3DH_CTRL_REG4_HR;
	_divider = 16380;

	error = i2c_write_register(i2c, address, LIS3DH_CTRL_REG4, _default_ctrl4_reg);if (error) return error;

	error = i2c_write_register(i2c, address, LIS3DH_CTRL_REG5, 0x00);if (error) return error;

	delay(20);

	vtmr_start(2500, _acquire, NULL);

	if (xTaskCreatePinnedToCore(_acquire_task, "LIS3DH", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, unit, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &task, xPortGetCoreID()) == pdFALSE) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	return NULL;
}

driver_error_t *LIS3DH_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	driver_error_t *error;
	uint8_t i2c = unit->setup[0].i2c.id;
	int16_t address = unit->setup[0].i2c.devid;

	// Start measuring
	error = i2c_write_register(i2c, address, LIS3DH_CTRL_REG1, _default_ctrl1_reg | LIS3DH_CTRL_REG1_ODR_400_HZ);if (error) return error;

	// Wait for data available
	uint8_t status;

	error = i2c_read_register(i2c, address, LIS3DH_STATUS_REG, &status);if (error) return error;
	while ((status & 0x04) != 0x04) {
		error = i2c_read_register(i2c, address, LIS3DH_STATUS_REG, &status);if (error) return error;
	}

	// Read acceleration in auto increment mode
	uint8_t buffer[6];

	error = i2c_read_register_block(i2c, address, 0x80 | LIS3DH_OUT_X_L, buffer, sizeof(buffer));if (error) return error;

	// Get RAW data
	int16_t x_raw;
	int16_t y_raw;
	int16_t z_raw;

	x_raw = ((buffer[1] << 8) | buffer[0]);
	y_raw = ((buffer[3] << 8) | buffer[2]);
	z_raw = ((buffer[5] << 8) | buffer[4]);

	// Get G data
	float x_g;
	float y_g;
	float z_g;

	x_g = ((float)x_raw) / ((float)_divider);
	y_g = ((float)y_raw) / ((float)_divider);
	z_g = ((float)z_raw) / ((float)_divider);

	printf("raw(%d, %d, %d), f(%f, %f, %f)\r\n", x_raw, y_raw, z_raw, x_g, y_g, z_g);

	values[0].floatd.value = x_g;
	values[1].floatd.value = y_g;
	values[2].floatd.value = z_g;

	return NULL;
}

driver_error_t *LIS3DH_set(sensor_instance_t *unit, const char *id, sensor_value_t *setting) {
	driver_error_t *error;
	uint8_t i2c = unit->setup[0].i2c.id;
	int16_t address = unit->setup[0].i2c.devid;

	if (strcmp(id,"full_scale") == 0) {
		if ((setting->integerd.value != 2) && (setting->integerd.value != 4) && (setting->integerd.value != 8) && (setting->integerd.value != 16)) {
			return driver_error(SENSOR_DRIVER, SENSOR_ERR_INVALID_VALUE, NULL);
		}

		uint8_t ctrl4_reg  = _default_ctrl4_reg;

		// Put sensor in power-down mode
		error = i2c_write_register(i2c, address, LIS3DH_CTRL_REG1, _default_ctrl1_reg);if (error) return error;

		// Set full scale range
		ctrl4_reg &= ~(LIS3DH_CTRL_REG4_FS_MASK);

		switch (setting->integerd.value) {
			case 2:  ctrl4_reg |= LIS3DH_CTRL_REG4_FS_2;  break;
			case 4:  ctrl4_reg |= LIS3DH_CTRL_REG4_FS_4;  break;
			case 8:  ctrl4_reg |= LIS3DH_CTRL_REG4_FS_8;  break;
			case 16: ctrl4_reg |= LIS3DH_CTRL_REG4_FS_16; break;
		}

		error = i2c_write_register(i2c, address, LIS3DH_CTRL_REG4, ctrl4_reg);if (error) return error;

		// Compute divider to make raw to g conversion
		switch (setting->integerd.value) {
			case 2:  _divider = 16384; break;
			case 4:  _divider = 8192;  break;
			case 8:  _divider = 4096;  break;
			case 16: _divider = 1365;  break;
		}
	}

	return NULL;
}

#endif
#endif

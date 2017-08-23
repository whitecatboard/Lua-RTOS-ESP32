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

#ifndef _SENSORS_H_
#define _SENSORS_H_

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR

#include <time.h>
#include <sys/time.h>

#include <sys/driver.h>
#include <drivers/adc.h>
#include <drivers/gpio.h>
#include <drivers/gpio_debouncing.h>

#define SENSOR_FAMILY_TEMP "Temperature"
#define SENSOR_FAMILY_HUM  "Humidity"

struct sensor_instance;
struct sensor_value;

// Sensor specific function types
typedef driver_error_t *(*sensor_setup_f_t)(struct sensor_instance *);
typedef driver_error_t *(*sensor_acquire_f_t)(struct sensor_instance *, struct sensor_value *);
typedef driver_error_t *(*sensor_set_f_t)(struct sensor_instance *, const char *, struct sensor_value *);
typedef driver_error_t *(*sensor_get_f_t)(struct sensor_instance *, const char *, struct sensor_value *);

#define SENSOR_MAX_DATA       6
#define SENSOR_MAX_PROPERTIES 4
#define SENSOR_MAX_CALLBACKS  4

#define SENSOR_FLAG_ON_OFF          (1 << 0)
#define SENSOR_FLAG_ON_H            (1 << 1)
#define SENSOR_FLAG_ON_L            (1 << 2)
#define SENSOR_FLAG_DEBOUNCING      (1 << 3)
#define SENSOR_FLAG_AUTO_ACQ        (1 << 4)

// Sensor interface
typedef enum {
	ADC_INTERFACE,
	SPI_INTERFACE,
	I2C_INTERFACE,
	OWIRE_INTERFACE,
	GPIO_INTERFACE,
	UART_INTERFACE
} sensor_interface_t;

// Sensor data type
typedef enum {
	SENSOR_NO_DATA,
	SENSOR_DATA_INT,
	SENSOR_DATA_FLOAT,
	SENSOR_DATA_DOUBLE,
	SENSOR_DATA_STRING,
} sensor_data_type_t;

// Sensor data
typedef struct {
	const char *id;
	const sensor_data_type_t type;
} sensor_data_t;

// Sensor property
typedef struct {
	const char *id;
	const sensor_data_type_t type;
} sensor_property_t;

// Sensor structure
typedef struct {
	const char *id;
	const sensor_interface_t interface;
	const uint32_t flags;
	const sensor_data_t data[SENSOR_MAX_DATA];
	const sensor_data_t properties[SENSOR_MAX_PROPERTIES];
	const sensor_setup_f_t setup;
	const sensor_setup_f_t presetup;
	const sensor_acquire_f_t acquire;
	const sensor_set_f_t set;
	const sensor_get_f_t get;
} sensor_t;

typedef struct sensor_value {
	sensor_data_type_t type;

	union {
		struct {
			int32_t value;
		} integerd;

		struct {
			float value;
		} floatd;

		struct {
			double value;
		} doubled;

		struct {
			char *value;
		} stringd;

		struct {
			int64_t value;
		} raw;
	};
} sensor_value_t;

// Sensor setup structure
typedef struct {
	union {
		struct {
			int8_t gpio;
		} gpio;

		struct {
			uint8_t unit;
			uint8_t channel;
			int16_t devid;
			uint8_t resolution;
			int16_t vrefp;
			int16_t vrefn;
			adc_channel_h_t h;
		} adc;

		struct {
			int8_t gpio;
			uint8_t owdevice;
			uint64_t owsensor;
		} owire;

		struct {
			uint8_t  id;
			uint32_t speed;
			int8_t  sda;
			int8_t  scl;
			int16_t devid;
			void     *userdata;
		} i2c;
		struct {
			uint8_t  id;
			uint32_t speed;
			uint8_t	 data_bits;
			uint8_t  parity;
			uint8_t  stop_bits;
		} uart;
	};
} sensor_setup_t;

struct sensor_instance;

// Sensor callback
typedef void (*sensor_callback_t)(int, struct sensor_instance *, sensor_value_t *);

// Sensor instance
typedef struct sensor_instance {
	int unit;
	sensor_value_t data[SENSOR_MAX_DATA];
	sensor_value_t properties[SENSOR_MAX_PROPERTIES];
	struct timeval next;

	struct {
		sensor_callback_t callback;
		int callback_id;
	} callbacks[SENSOR_MAX_CALLBACKS];

	const sensor_t *sensor;
	sensor_setup_t setup;
	sensor_setup_t presetup;
} sensor_instance_t;

typedef struct {
	sensor_instance_t *instance;
	sensor_callback_t callback;
	sensor_value_t data[SENSOR_MAX_DATA];
	int callback_id;
} sensor_deferred_data_t;

const sensor_t *get_sensor(const char *id);
const sensor_data_t *sensor_get_property(const sensor_t *sensor, const char *property);
driver_error_t *sensor_setup(const sensor_t *sensor, sensor_setup_t *setup, sensor_instance_t **unit);
driver_error_t *sensor_unsetup(sensor_instance_t *unit);
driver_error_t *sensor_acquire(sensor_instance_t *unit);
driver_error_t *sensor_read(sensor_instance_t *unit, const char *id, sensor_value_t **value);
driver_error_t *sensor_set(sensor_instance_t *unit, const char *id, sensor_value_t *value);
driver_error_t *sensor_get(sensor_instance_t *unit, const char *id, sensor_value_t **value);
driver_error_t *sensor_register_callback(sensor_instance_t *unit, sensor_callback_t callback, int id, uint8_t deferred);

// SENSOR errors
#define SENSOR_ERR_CANT_INIT                (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  0)
#define SENSOR_ERR_TIMEOUT                  (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  1)
#define SENSOR_ERR_NOT_ENOUGH_MEMORY		(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  2)
#define SENSOR_ERR_SETUP_UNDEFINED		    (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  3)
#define SENSOR_ERR_ACQUIRE_UNDEFINED		(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  4)
#define SENSOR_ERR_SET_UNDEFINED		    (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  5)
#define SENSOR_ERR_NOT_FOUND				(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  6)
#define SENSOR_ERR_INTERFACE_NOT_SUPPORTED	(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  7)
#define SENSOR_ERR_NOT_SETUP				(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  8)
#define SENSOR_ERR_INVALID_ADDRESS			(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  9)
#define SENSOR_ERR_NO_MORE_CALLBACKS		(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) | 10)
#define SENSOR_ERR_INVALID_DATA				(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) | 11)

#endif

#endif /* _SENSORS_H_ */

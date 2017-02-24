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

#include "luartos.h"

#if USE_SENSORS

#include <sys/driver.h>

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

// Sensor interface
typedef enum {
	ADC_INTERFACE,
	SPI_INTERFACE,
	I2C_INTERFACE,
	OWIRE_INTERFACE,
	GPIO_INTERFACE
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
	const sensor_data_t data[SENSOR_MAX_DATA];
	const sensor_data_t properties[SENSOR_MAX_PROPERTIES];
	const sensor_setup_f_t setup;
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
			uint8_t gpio;
		} gpio;

		struct {
			uint8_t unit;
			uint8_t channel;
			uint8_t resolution;
		} adc;

		struct {
			uint8_t gpio;
			uint8_t owdevice;
			uint64_t owsensor;
		} owire;

		struct {
			uint8_t  id;
			uint16_t speed;
			uint8_t  sda;
			uint8_t  scl;
			uint16_t address;
		} i2c;
	};
} sensor_setup_t;

// Sensor instance
typedef struct sensor_instance {
	int unit;
	sensor_value_t data[SENSOR_MAX_DATA];
	sensor_value_t properties[SENSOR_MAX_PROPERTIES];
	const sensor_t *sensor;
	sensor_setup_t setup;
} sensor_instance_t;

const sensor_t *get_sensor(const char *id);
const sensor_data_t *sensor_get_property(const sensor_t *sensor, const char *property);
driver_error_t *sensor_setup(const sensor_t *sensor, sensor_setup_t *setup, sensor_instance_t **unit);
driver_error_t *sensor_acquire(sensor_instance_t *unit);
driver_error_t *sensor_read(sensor_instance_t *unit, const char *id, sensor_value_t **value);
driver_error_t *sensor_set(sensor_instance_t *unit, const char *id, sensor_value_t *value);
driver_error_t *sensor_get(sensor_instance_t *unit, const char *id, sensor_value_t **value);

// SENSOR errors
#define SENSOR_ERR_CANT_INIT                (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  0)
#define SENSOR_ERR_TIMEOUT                  (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  1)
#define SENSOR_ERR_NOT_ENOUGH_MEMORY		(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  2)
#define SENSOR_ERR_SETUP_UNDEFINED		    (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  3)
#define SENSOR_ERR_ACQUIRE_UNDEFINED		(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  4)
#define SENSOR_ERR_SET_UNDEFINED		    (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  5)
#define SENSOR_ERR_NOT_FOUND				(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  6)
#define SENSOR_ERR_INTERFACE_NOT_SUPPORTED	(DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID) |  7)

#endif

#endif /* _SENSORS_H_ */

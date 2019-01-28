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
 * Lua RTOS, sensor driver
 *
 */

#ifndef _SENSORS_H_
#define _SENSORS_H_

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR

#include <time.h>
#include <sys/time.h>

#include <sys/mutex.h>
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
typedef driver_error_t *(*sensor_unsetup_f_t)(struct sensor_instance *);
typedef driver_error_t *(*sensor_acquire_f_t)(struct sensor_instance *, struct sensor_value *);
typedef driver_error_t *(*sensor_set_f_t)(struct sensor_instance *, const char *, struct sensor_value *);
typedef driver_error_t *(*sensor_get_f_t)(struct sensor_instance *, const char *, struct sensor_value *);

extern const int sensor_errors;
extern const int sensor_error_map;

#define SENSOR_MAX_INTERFACES 8
#define SENSOR_MAX_PROPERTIES 16
#define SENSOR_MAX_CALLBACKS  8

/*
 * Sensor flags. Each sensor has a definition flag that stores information about
 * how sensor acquires it's information.
 */

// bit 0
// Sensor initializes it's interfaces by it self. Don't use default initialization.
#define SENSOR_FLAG_CUSTOM_INTERFACE_INIT (1 << 0)

// bit 1
// Sensor acquires data automatically. This type of sensors acquire data when an
// interrupt is raised, when a time interval expires, or when sensor sends a new
// data.
#define SENSOR_FLAG_AUTO_ACQ (1 << 1)

// bit 2
// Sensor acquires data from a GPIO interrupt.
#define SENSOR_FLAG_ON_OFF (1 << 2)

// bits 3, 4, 5
// If SENSOR_FLAG_ON_OFF is enabled, this flag hold the property value when GPIO is
// in the high state.
#define SENSOR_FLAG_ON_H(value) (value << 3)
#define SENSOR_FLAG_GET_ON_H(interface) ((interface.flags & 0x38) >> 3)

// bits 6, 7, 8
// If SENSOR_FLAG_ON_OFF is enabled, this flag hold the property value when GPIO is
// in the low state.
#define SENSOR_FLAG_ON_L(value) (value << 6)
#define SENSOR_FLAG_GET_ON_L(interface) ((interface.flags & 0x1c0) >> 6)

// bit 9
// If SENSOR_FLAG_ON_OFF is enabled this flag controls if software debouncing techniques
// must be applied.
#define SENSOR_FLAG_DEBOUNCING (1 << 9)

// bits 10, 11, 12, 13, 14
// If SENSOR_FLAG_AUTO_ACQ this flag controls in which property the sensor must store it's
// data.
#define SENSOR_FLAG_PROPERTY(prop) (prop << 10)
#define SENSOR_FLAG_GET_PROPERTY(interface) ((interface.flags & 0X7C00) >> 10)

// bit 15
#define SENSOR_FLAG_DEBOUNCING_THRESHOLD(value) (value << 15)
#define SENSOR_FLAG_GET_DEBOUNCING_THRESHOLD(interface) ((interface.flags & 0x1ffff8000) >> 15)

// Sensor interface types
typedef enum {
    ADC_INTERFACE = 1,
    SPI_INTERFACE,
    I2C_INTERFACE,
    OWIRE_INTERFACE,
    GPIO_INTERFACE,
    UART_INTERFACE,
    INTERNAL_INTERFACE
} sensor_interface_type_t;


typedef struct {
    sensor_interface_type_t type;
    uint32_t flags;
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
    uint32_t flags;
} sensor_data_t;

// Sensor property
typedef struct {
    const char *id;
    const sensor_data_type_t type;
} sensor_property_t;

// Sensor structure
typedef struct {
    const char *id;
    const sensor_interface_t interface[SENSOR_MAX_INTERFACES];
    const char* interface_name[SENSOR_MAX_INTERFACES];
    const sensor_data_t data[SENSOR_MAX_PROPERTIES];
    const sensor_data_t properties[SENSOR_MAX_PROPERTIES];
    const sensor_setup_f_t setup;
    const sensor_setup_f_t presetup;
    const sensor_unsetup_f_t unsetup;
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

typedef struct {
    uint8_t timeout;
    uint8_t repeat;
    struct timeval t;
    sensor_value_t value;
} sensor_latch_t;

// Sensor setup structure
typedef struct {
    uint8_t interface;
    void *instance;

    union {
        struct {
            int8_t gpio;
        } gpio;

        struct {
            uint8_t unit;
            uint8_t channel;
            int16_t devid;
            uint8_t resolution;
            int16_t max;
            adc_channel_h_t h;
        } adc;

        struct {
            int8_t gpio;
            uint8_t owdevice;
            uint64_t owsensor;
        } owire;

        struct {
            uint16_t  id;
            uint32_t speed;
            int8_t  sda;
            int8_t  scl;
            int16_t devid;
            void     *userdata;
        } i2c;
        struct {
            uint8_t  id;
            uint32_t speed;
            uint8_t     data_bits;
            uint8_t  parity;
            uint8_t  stop_bits;
        } uart;
    };
} sensor_setup_t;

struct sensor_instance;

// Sensor callback
typedef void (*sensor_callback_t)(int, struct sensor_instance *, sensor_value_t *, sensor_latch_t *);

// Sensor instance
typedef struct sensor_instance {
    int unit;
    struct mtx mtx;
    sensor_value_t data[SENSOR_MAX_PROPERTIES];
    sensor_latch_t latch[SENSOR_MAX_PROPERTIES];
    sensor_value_t properties[SENSOR_MAX_PROPERTIES];
    struct timeval next;

    struct {
        sensor_callback_t callback;
        int callback_id;
    } callbacks[SENSOR_MAX_CALLBACKS];

    const sensor_t *sensor;
    sensor_setup_t setup[SENSOR_MAX_INTERFACES];
    void *args;
} sensor_instance_t;

typedef struct {
    sensor_instance_t *instance;
    sensor_callback_t callback;
    sensor_value_t data[SENSOR_MAX_PROPERTIES];
    sensor_latch_t latch[SENSOR_MAX_PROPERTIES];
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
void sensor_queue_callbacks(sensor_instance_t *unit, uint8_t from, uint8_t to);
void sensor_init_data(sensor_instance_t *unit);
void sensor_update_data(sensor_instance_t *unit, uint8_t from, uint8_t to, sensor_value_t *new_data, uint64_t delay, uint64_t rate, uint8_t ignore, uint64_t ignore_val);
void IRAM_ATTR sensor_lock(sensor_instance_t *unit);
void IRAM_ATTR sensor_unlock(sensor_instance_t *unit);

// SENSOR errors
#define SENSOR_ERR_CANT_INIT                (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  |  0)
#define SENSOR_ERR_TIMEOUT                  (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  |  1)
#define SENSOR_ERR_NOT_ENOUGH_MEMORY        (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  |  2)
#define SENSOR_ERR_SETUP_UNDEFINED          (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  |  3)
#define SENSOR_ERR_ACQUIRE_UNDEFINED        (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  |  4)
#define SENSOR_ERR_SET_UNDEFINED            (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  |  5)
#define SENSOR_ERR_NOT_FOUND                (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  |  6)
#define SENSOR_ERR_INTERFACE_NOT_SUPPORTED  (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  |  7)
#define SENSOR_ERR_NOT_SETUP                (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  |  8)
#define SENSOR_ERR_INVALID_ADDRESS          (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  |  9)
#define SENSOR_ERR_NO_MORE_CALLBACKS        (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  | 10)
#define SENSOR_ERR_INVALID_DATA             (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  | 11)
#define SENSOR_ERR_CALLBACKS_NOT_ALLOWED    (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  | 12)
#define SENSOR_ERR_INVALID_VALUE            (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  | 13)
#define SENSOR_ERR_DETACHED                 (DRIVER_EXCEPTION_BASE(SENSOR_DRIVER_ID)  | 14)
#endif

#endif /* _SENSORS_H_ */

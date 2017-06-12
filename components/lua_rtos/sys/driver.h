/*
 * Lua RTOS, driver basics
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

#ifndef DRIVER_H
#define DRIVER_H

#include "luartos.h"
#include "lobject.h"
#include <modules.h>

#include <sys/list.h>
#include <sys/resource.h>
#include <sys/driver.h>

#define DRIVER_ALL_FLAGS   0xff

#define ADC_DRIVER_ID      1
#define GPIO_DRIVER_ID     2
#define I2C_DRIVER_ID      3
#define UART_DRIVER_ID     4
#define SPI_DRIVER_ID      5
#define LORA_DRIVER_ID     6
#define PWM_DRIVER_ID      7
#define WIFI_DRIVER_ID     8
#define NET_DRIVER_ID      9
#define SENSOR_DRIVER_ID   10
#define OWIRE_DRIVER_ID    11
#define MQTT_DRIVER_ID     12
#define SERVO_DRIVER_ID    13
#define THREAD_DRIVER_ID   14
#define PWBUS_DRIVER_ID    15
#define NZR_DRIVER_ID      16
#define NEOPIXEL_DRIVER_ID 17
#define STEPPER_DRIVER_ID  18
#define TM1637_DRIVER_ID   19
#define EVENT_DRIVER_ID    20
#define SPI_ETH_DRIVER_ID  21
#define CAN_DRIVER_ID      22
#define SPI_SD_DRIVER_ID   23

#define GPIO_DRIVER driver_get_by_name("gpio")
#define UART_DRIVER driver_get_by_name("uart")
#define SPI_DRIVER driver_get_by_name("spi")
#define I2C_DRIVER driver_get_by_name("i2c")
#define SENSOR_DRIVER driver_get_by_name("sensor")
#define ADC_DRIVER driver_get_by_name("adc")
#define MQTT_DRIVER driver_get_by_name("mqtt")
#define OWIRE_DRIVER driver_get_by_name("owire")
#define SERVO_DRIVER driver_get_by_name("servo")
#define NZR_DRIVER driver_get_by_name("nzr")
#define NEOPIXEL_DRIVER driver_get_by_name("neopixel")
#define STEPPER_DRIVER driver_get_by_name("stepper")
#define TM1637_DRIVER driver_get_by_name("tm1637")
#define EVENT_DRIVER driver_get_by_name("event")
#define SPI_ETH_DRIVER driver_get_by_name("spi_eth")
#define CAN_DRIVER driver_get_by_name("can")
#define PWBUS_DRIVER driver_get_by_name("pwbus")
#define SPI_SD_DRIVER driver_get_by_name("spi_sd")

#define DRIVER_EXCEPTION_BASE(n) (n << 24)

struct driver;
struct driver_error;
struct driver_unit_lock;
struct driver_unit_lock_error;

typedef struct {
	int exception;
	const char *message;
} driver_message_t;

typedef enum {
    LOCK,		// Someone needs a resource which is locked
    SETUP,      // Something fails during setup
	OPERATION   // Something fails during normal operation
} driver_error_type;


typedef struct {
    driver_error_type       type;      // Type of error
    const struct driver    *driver;    // Driver that caused error
    int                     unit;      // Driver unit that caused error
    int                     exception; // Exception code
    const char 			   *msg;       // Error message

    struct driver_unit_lock_error *lock_error;
} driver_error_t;

/**
 * @brief This structure maintains basic information about a
 *        driver, such as the driver name, the error messages for
 *        each exception, etc ...
 */
typedef struct driver {
	const char *name;           		  /*!< Driver name */
	const int  exception_base;  	      /*!< The exception base number for this driver. When a exception is raised the exception number is exception_base + exception number */
	const driver_message_t *error;        /*!< Array of exception error messages */
	const struct driver_unit_lock *lock;  /*!< Array locks */
	const int locks;					  /*!< Number of locks */
	void (*init)();             		  /*!< Driver initialization functions, called at system init */

	// Driver lock function
	driver_error_t *(*lock_resources)(int,uint8_t, void *);
} driver_t;

/**
 * @brief This structure maintains information about a lock. Drivers
 *        usually define an array of driver_unit_lock_t structures
 *        (one for each driver unit), and each array entry keep the
 *        driver unit that owns the lock.
 *
 *        Example:
 *
 *        The GPIO driver defines the gpio_locks array,
 *        with one entry for each GPIO.
 *
 *        Imagine that the UART driver requires a lock on GPIO15 (rx pin)
 *        and on GPIO14 (tx pin) for the UART0.This information is stored
 *        in gpio_locks[14] and gpio_locks[15], with the following information:
 *
 *        gpio_locks[14] = {pointer to UART driver structure, 0}
 *        gpio_locks[15] = {pointer to UART driver structure, 0}
 *
 *        gpio_locks[14] says that GPIO14 is owned by UART0.
 *        gpio_locks[15] says that GPIO15 is owned by UART0.
 */
typedef struct driver_unit_lock {
	const driver_t *owner; /*!< Which driver owns the lock */
	int unit;			   /*!< Which unit driver owns the lock */
	const char *tag;       /*!< A tag, used, for example for set the signal name */
} driver_unit_lock_t;

typedef struct driver_unit_lock_error {
	driver_unit_lock_t *lock;

	const driver_t *owner_driver;
	int owner_unit;

	const driver_t *target_driver;
	int target_unit;
} driver_unit_lock_error_t;

/**
 * @brief Search for a driver into the driver's array by name and get
 *        the driver structure.
 *
 * @param name Driver name.
 *
 * @return NULL if driver not found, or a pointer to a driver_t structure
 *         if driver is found.
 */
const driver_t *driver_get_by_name(const char *name);

/**
 * @brief Search for a driver into the driver's array by exception base
 *        and get the driver structure.
 *
 * @param exception_base Driver exception base.
 *
 * @return NULL if driver not found, or a pointer to a driver_t structure
 *         if driver is found.
 */
const driver_t *driver_get_by_exception_base(const int exception_base);

const char *driver_get_err_msg(driver_error_t *error);
const char *driver_get_err_msg_by_exception(int exception);
const char *driver_get_name(driver_error_t *error);

driver_error_t *driver_lock_error(const driver_t *driver, driver_unit_lock_error_t *lock_error);
driver_error_t *driver_setup_error(const driver_t *driver, unsigned int code, const char *msg);
driver_error_t *driver_operation_error(const driver_t *driver, unsigned int code, const char *msg);
driver_unit_lock_error_t *driver_lock(const driver_t *owner_driver, int owner_unit, const driver_t *target_driver, int target_unit, uint8_t flags, const char *tag);
void _driver_init();
char *driver_target_name(const driver_t *target_driver, int target_unit, const char *tag);

#define DRIVER_SECTION(s) __attribute__((used,unused,section(s)))

#define DRIVER_PASTER(x,y) x##y
#define DRIVER_EVALUATOR(x,y) DRIVER_PASTER(x,y)
#define DRIVER_CONCAT(x,y) DRIVER_EVALUATOR(x,y)

#define DRIVER_TOSTRING_PASTER(x) #x
#define DRIVER_TOSTRING_EVALUATOR(x) DRIVER_TOSTRING_PASTER(x)
#define DRIVER_TOSTRING(x) DRIVER_TOSTRING_EVALUATOR(x)

#define DRIVER_REGISTER(name,lname,locka,initf,lockf) \
	const DRIVER_SECTION(DRIVER_TOSTRING(.drivers)) driver_t DRIVER_CONCAT(driver_,lname) = {DRIVER_TOSTRING(lname),  DRIVER_EXCEPTION_BASE(DRIVER_CONCAT(name,_DRIVER_ID)),  (void *)DRIVER_CONCAT(lname,_errors), locka, ((locka!=NULL)?(sizeof(locka)/sizeof(driver_unit_lock_t)):0), initf, lockf};
#endif

#define DRIVER_REGISTER_ERROR(name, lname, key, msg, exception) \
	extern const driver_message_t DRIVER_CONCAT(lname,_errors)[]; \
	const __attribute__((used,unused,section(DRIVER_TOSTRING(DRIVER_CONCAT(.lname,_errors))))) driver_message_t DRIVER_CONCAT(lname,DRIVER_CONCAT(key,_errors)) = {exception, msg}; \
	const __attribute__((used,unused,section(DRIVER_TOSTRING(DRIVER_CONCAT(.lname,_error_map))))) LUA_REG_TYPE DRIVER_CONCAT(lname,DRIVER_CONCAT(key,_error_map)) = {LSTRKEY(DRIVER_TOSTRING(key)), LINTVAL(exception)};

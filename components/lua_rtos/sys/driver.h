/*
 * Lua RTOS, driver basics
 *
 * Copyright (C) 2015 - 2016
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

#ifndef DRIVER_H
#define DRIVER_H

#include "luartos.h"

#include <sys/list.h>
#include <sys/resource.h>
#include <sys/driver.h>

#define ADC_DRIVER_ID  1
#define GPIO_DRIVER_ID 2
#define UART_DRIVER_ID 3
#define SPI_DRIVER_ID  4
#define LORA_DRIVER_ID 5

#define DRIVER_EXCEPTION_BASE(n) (n << 24)

struct driver;
struct driver_error;
struct driver_unit_lock;
struct driver_unit_lock_error;

typedef enum {
    LOCK,		// Someone needs a resource which is locked
    SETUP,      // Something fails during setup
	OPERATION   // Something fails during normal operation
} driver_error_type;


typedef struct {
    driver_error_type    type;      // Type of error
    const struct driver *driver;    // Driver that caused error
    int                  unit;      // Driver unit that caused error
    int                  exception; // Exception code
    const char          *msg;       // Error message

    struct driver_unit_lock_error *lock_error;
} driver_error_t;

typedef struct driver {
	const char *name;           // Driver name
	const int  exception_base;  // Constant for add to driver errors (from 1 to n)
	const void *error;          // Error messages
	const struct driver_unit_lock *lock;           // Driver lock array
	void (*init)();             // Init function for driver that must be caller in system init

	driver_error_t *(*lock_resources)(int,void *);             // Init function for driver that must be caller in system init
} driver_t;

typedef struct driver_unit_lock {
	const driver_t *owner; // Who owns the lock
	int unit;
} driver_unit_lock_t;

typedef struct driver_unit_lock_error {
	driver_unit_lock_t *lock;

	const driver_t *owner_driver;
	int owner_unit;

	const driver_t *target_driver;
	int target_unit;
} driver_unit_lock_error_t;

const driver_t *driver_get(const char *name);
const char *driver_get_err_msg(driver_error_t *error);
const char *driver_get_name(driver_error_t *error);

driver_error_t *driver_lock_error(const driver_t *driver, driver_unit_lock_error_t *lock_error);
driver_error_t *driver_setup_error(const driver_t *driver, unsigned int code, const char *msg);
driver_error_t *driver_operation_error(const driver_t *driver, unsigned int code, const char *msg);
driver_unit_lock_error_t *driver_lock(const driver_t *owner_driver, int owner_unit, const driver_t *target_driver, int target_unit);
void _driver_init();

#define GPIO_DRIVER driver_get("gpio")
#define UART_DRIVER driver_get("uart")
#define SPI_DRIVER driver_get("spi")

#endif

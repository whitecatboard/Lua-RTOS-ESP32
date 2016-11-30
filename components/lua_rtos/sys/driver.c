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

#include "luartos.h"

#include <stdlib.h>
#include <string.h>

#include <sys/driver.h>

extern const char *uart_errors[];
extern const char *spi_errors[];
extern const char *lora_lmic_errors[];

const driver_t drivers[] = {
#if USE_UART
	{"uart", DRIVER_EXCEPTION_BASE(UART_DRIVER_ID), (void *)uart_errors},
#endif
#if USE_SPI
	{"spi", DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID), (void *)spi_errors},
#endif
#if USE_LMIC
	{"lora", DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID), (void *)lora_lmic_errors},
#endif
	{NULL, 0, NULL}
};

// Get driver info by it's name
const driver_t *driver_get(const char *name) {
	const driver_t *cdriver;

	cdriver = drivers;
	while (cdriver->name) {
		if (strcmp(name, cdriver->name) == 0) {
			return cdriver;
		}

		cdriver++;
	}

	return NULL;
}

// Get error message string fom a driver error
const char *driver_get_err_msg(driver_error_t *error) {
	return (const char *)(*(unsigned int *)(error->driver->error + sizeof(void *) * (~error->driver->exception_base & error->exception)));
}

// Get driver name from a driver error
const char *driver_get_name(driver_error_t *error) {
	return error->driver->name;
}

// Create a driver error of type lock from a lock structure
driver_error_t *driver_lock_error(resource_lock_t *lock) {
	driver_error_t *error;

    error = (driver_error_t *)malloc(sizeof(driver_error_t));
    if (error) {
        error->type = LOCK;
        error->resource = lock->type;
        error->resource_unit = lock->unit;
        error->owner = lock->owner;
        error->owner_unit = lock->owner_unit;
    }

    free(lock);

    return error;
}

// Create a driver error of type setup
driver_error_t *driver_setup_error(const driver_t *driver, unsigned int exception, const char *msg) {
	driver_error_t *error;

    error = (driver_error_t *)malloc(sizeof(driver_error_t));
    if (error) {
        error->type = SETUP;
        error->driver = driver;
        error->exception = exception;
        error->msg = msg;
    }

    return error;
}

// Create a driver error of type operation
driver_error_t *driver_operation_error(const driver_t *driver, unsigned int exception, const char *msg) {
	driver_error_t *error;

	error = (driver_error_t *)malloc(sizeof(driver_error_t));
    if (error) {
        error->type = OPERATION;
        error->driver = driver;
        error->exception = exception;
        error->msg = msg;
    }

    return error;
}


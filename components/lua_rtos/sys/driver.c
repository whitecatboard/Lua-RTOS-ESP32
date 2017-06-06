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

#include "luartos.h"

#include "lora.h"

#include <stdlib.h>
#include <string.h>

#include <sys/driver.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/mutex.h>

#include <drivers/pwm.h>
#include <drivers/adc.h>
#include <drivers/i2c.h>
#include <drivers/uart.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>

#define DRIVER_LOCK_DEBUG 0

// Mutex for lock resources
static struct mtx driver_mtx;

// This is provided by linker
// Drivers are registered in their soure code file using DRIVER_REGISTER macro
extern const driver_t drivers[];

// Get driver info by it's name
const driver_t *driver_get_by_name(const char *name) {
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

// Get driver info by it's exception base
const driver_t *driver_get_by_exception_base(const int exception_base) {
	const driver_t *cdriver;

	cdriver = drivers;
	while (cdriver->name) {
		if (cdriver->exception_base == exception_base) {
			return cdriver;
		}

		cdriver++;
	}

	return NULL;
}

// Get error message string fom a driver error
const char *driver_get_err_msg(driver_error_t *error) {
	driver_message_t *msg = (driver_message_t *)error->driver->error;

	while (msg->message) {
		if (msg->exception == error->exception) {
			return msg->message;
		}

		msg++;
	}

	return NULL;
}

const char *driver_get_err_msg_by_exception(int exception) {
	const driver_t *driver;
	const driver_message_t *msg;

	// Get driver by name
	driver = driver_get_by_exception_base(exception & 0b11111111000000000000000000000000);
	if (driver) {
		msg = driver->error;

		while (msg->message) {
			if (msg->exception == exception) {
				return msg->message;
			}

			msg++;
		}
	}

	return NULL;
}

// Get driver name from a driver error
const char *driver_get_name(driver_error_t *error) {
	return error->driver->name;
}

// Create a driver error of type lock from a lock structure
driver_error_t *driver_lock_error(const driver_t *driver, driver_unit_lock_error_t *lock_error) {
	driver_error_t *error;

    error = (driver_error_t *)malloc(sizeof(driver_error_t));
    if (error) {
        error->type = LOCK;
        error->lock_error = lock_error;
    }

    free(lock_error);

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

// Try to obtain a lock on an unit driver
driver_unit_lock_error_t *driver_lock(const driver_t *owner_driver, int owner_unit, const driver_t *target_driver, int target_unit) {
    mtx_lock(&driver_mtx);

	driver_unit_lock_t *target_lock = (driver_unit_lock_t *)target_driver->lock;

	#if DRIVER_LOCK_DEBUG
	syslog(LOG_DEBUG,"driver lock by %s%d on %s%d\r\n",
			owner_driver->name, owner_unit,
			target_driver->name, target_unit
	);
	#endif

	if (!target_lock) {
		#if DRIVER_LOCK_DEBUG
		syslog(LOG_DEBUG,"target driver haven't lock control\r\n");
		#endif

		if (target_driver->lock_resources) {
			driver_error_t *error;

			mtx_unlock(&driver_mtx);

			if ((error = target_driver->lock_resources(target_unit, NULL))) {
				// Target driver has no locks, then grant access
				#if DRIVER_LOCK_DEBUG
				syslog(LOG_DEBUG,"driver lock by %s%d on %s%d revoked\r\n",
						owner_driver->name, owner_unit,
						target_driver->name, target_unit
				);
				#endif

				return error->lock_error;
			} else {
				// Target driver has no locks, then grant access
				#if DRIVER_LOCK_DEBUG
				syslog(LOG_DEBUG,"driver lock by %s%d on %s%d granted\r\n",
						owner_driver->name, owner_unit,
						target_driver->name, target_unit
				);
				#endif

				return NULL;
			}
		}

		// Target driver has no locks, then grant access
		#if DRIVER_LOCK_DEBUG
		syslog(LOG_DEBUG,"driver lock by %s%d on %s%d granted\r\n",
				owner_driver->name, owner_unit,
				target_driver->name, target_unit
		);
		#endif

		mtx_unlock(&driver_mtx);

		return NULL;
	} else {
		#if DRIVER_LOCK_DEBUG
		syslog(LOG_DEBUG,"target driver have lock control\r\n");
		#endif
	}

	if (target_lock[target_unit].owner) {
		// Target unit has a lock

		if ((target_lock[target_unit].owner == owner_driver) && ((target_lock[target_unit].unit == owner_unit))) {
			// Target unit is locked by owner_driver, then grant access
			#if DRIVER_LOCK_DEBUG
			syslog(LOG_DEBUG,"driver lock by %s%d on %s%d granted\r\n",
					owner_driver->name, owner_unit,
					target_driver->name, target_unit
			);
			#endif

			mtx_unlock(&driver_mtx);

			return NULL;
		} else {
			// Target unit is locked by another driver, then revoke access
			driver_unit_lock_error_t *error = (driver_unit_lock_error_t *)malloc(sizeof(driver_unit_lock_error_t));
			if (!error) {
				panic("not enough memory");
			}

			error->lock = &target_lock[target_unit];
			error->owner_driver = owner_driver;
			error->owner_unit = owner_unit;
			error->target_driver = target_driver;
			error->target_unit = target_unit;

			#if DRIVER_LOCK_DEBUG
			syslog(LOG_DEBUG,"driver lock by %s%d on %s%d revoked\r\n",
					owner_driver->name, owner_unit,
					target_driver->name, target_unit
			);
			#endif

			mtx_unlock(&driver_mtx);

			return error;
		}
	} else {
		#if DRIVER_LOCK_DEBUG
		syslog(LOG_DEBUG,"driver lock by %s%d on %s%d granted\r\n",
				owner_driver->name, owner_unit,
				target_driver->name, target_unit
		);
		#endif

		// Target unit hasn't a lock, then grant access
		target_lock[target_unit].owner = owner_driver;
		target_lock[target_unit].unit = owner_unit;

		mtx_unlock(&driver_mtx);

		return NULL;
	}
}

void _driver_init() {
    // Create driver mutex
    mtx_init(&driver_mtx, NULL, NULL, 0);

    // Init drivers
    const driver_t *cdriver = drivers;

    while (cdriver->name) {
    	if (cdriver->init) {
    		cdriver->init();
    	}
    	cdriver++;
    }
}

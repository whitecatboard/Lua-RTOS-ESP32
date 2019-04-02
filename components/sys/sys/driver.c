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
 * Lua RTOS driver common functions
 *
 */

#include "luartos.h"

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

const driver_t *driver_get_by_exception_base(const uint32_t exception_base) {
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

const char *driver_get_err_msg_by_exception(uint32_t exception) {
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

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
// Create a driver error of type lock from a lock structure
driver_error_t *driver_lock_error(const driver_t *driver, driver_unit_lock_error_t *lock_error) {
	driver_error_t *error;

	// Unlock all resources locked by the driver
    driver_unlock_all(driver, lock_error->owner_unit);

    error = (driver_error_t *)malloc(sizeof(driver_error_t));
    if (error) {
        error->type = LOCK;
        error->lock_error = lock_error;
    }

    return error;
}
#endif

// Create a driver error
driver_error_t *driver_error(const driver_t *driver, uint32_t exception, const char *msg) {
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

char *driver_target_name(const driver_t *target_driver, int target_unit, const char *tag) {
	uint8_t unit;

	// In some cases the target driver can have more than one device
	// attached. This is the case of the SPI driver, in which a SPI unit
	// (spi bus) have one or more devices attached.
	//
	// In this case we have to decode target_unit to obtain the unit.
	if (strcmp(target_driver->name, "spi") == 0) {
		unit = (target_unit & 0xff00) >> 8;
	} else if (strcmp(target_driver->name, "i2c") == 0) {
		unit = (target_unit & 0xff00) >> 8;
	} else {
		unit = target_unit;
	}

	char *buffer = malloc(80);
	if (!buffer) {
		panic("not enough memory");
	}

	if (tag) {
		snprintf(buffer, 40, "%s%d (%s)", target_driver->name, unit, tag);
	} else {
		snprintf(buffer, 40, "%s%d", target_driver->name, unit);
	}

	return buffer;
}

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
static int lock_index(const driver_t *driver, int unit) {
	int tunit;
	int tdevice;
	int index;

	// In some cases the target driver can have more than one device
	// attached. This is the case of the SPI driver, in which a SPI unit
	// (spi bus) have one or more devices attached.
	//
	// In this case we split the target_unit into the target_unit and the
	// target_device.
	if (strcmp(driver->name, "spi") == 0) {
		tunit = (unit & 0xff00) >> 8;
		tdevice = (unit & 0x00ff);
		index = (tunit * SPI_BUS_DEVICES) + tdevice;
	} else if (strcmp(driver->name, "i2c") == 0) {
		tunit = (unit & 0xff00) >> 8;
		tdevice = (unit & 0x00ff);
		index = (tunit * I2C_BUS_DEVICES) + tdevice;
	} else {
		index = unit;
	}

	return index;
}

void driver_unlock_all(const driver_t *owner_driver, int owner_unit) {
    const driver_t *cdriver = drivers;
    driver_unit_lock_t *target_lock;
    int i;

    mtx_lock(&driver_mtx);

    while (cdriver->name) {
        if (*cdriver->lock) {
            target_lock = (driver_unit_lock_t *)*cdriver->lock;
            for(i=0; i < cdriver->locks;i++) {
                if ((target_lock[i].owner == owner_driver) && (target_lock[i].unit == owner_unit)) {
                    target_lock[i].owner = NULL;
                    target_lock[i].unit = 0;
                    target_lock[i].tag = NULL;
                }
            }
        }

        cdriver++;
    }

	mtx_unlock(&driver_mtx);
}

// Try to obtain a lock on an unit driver
driver_unit_lock_error_t *driver_lock(const driver_t *owner_driver, int owner_unit, const driver_t *target_driver, int target_unit, uint8_t flags, const char *tag) {
	mtx_lock(&driver_mtx);

	if (strcmp(owner_driver->name, "spi") == 0) {
		owner_unit = (owner_unit << 8);
	}

    // Get the target driver lock array
	driver_unit_lock_t *target_lock = (driver_unit_lock_t *)*target_driver->lock;
	if ((!target_lock) && (target_driver->locks > 0)) {
	    // Allocate space for locks
	    target_lock = calloc(target_driver->locks, sizeof(driver_unit_lock_t));
	    assert(target_lock != NULL);

	    *target_driver->lock = target_lock;
	}

	#if DRIVER_LOCK_DEBUG
	char *name = driver_target_name(target_driver, target_unit, NULL);

	syslog(LOG_DEBUG,"driver lock by %s%d on %s\r\n",
			owner_driver->name, owner_unit, name
	);

	#endif

	if (!target_lock) {
		#if DRIVER_LOCK_DEBUG
		syslog(LOG_DEBUG,"driver %s haven't lock control\r\n", target_driver->name);
		#endif

		if (target_driver->lock_resources) {
			driver_error_t *error;

			mtx_unlock(&driver_mtx);

			if ((error = target_driver->lock_resources(target_unit, flags, NULL))) {
				// Target driver has no locks, then grant access
				#if DRIVER_LOCK_DEBUG
				syslog(LOG_DEBUG,"driver lock by %s%d on %s revoked\r\n",
						owner_driver->name, owner_unit,
						name
				);
				free(name);
				#endif

				driver_unlock_all(owner_driver, owner_unit);

				return error->lock_error;
			} else {
				// Target driver has no locks, then grant access
				#if DRIVER_LOCK_DEBUG
				syslog(LOG_DEBUG,"driver lock by %s%d on %s granted\r\n",
						owner_driver->name, owner_unit,
						name
				);
				free(name);
				#endif

				return NULL;
			}
		}

		// Target driver has no locks, then grant access
		#if DRIVER_LOCK_DEBUG
		syslog(LOG_DEBUG,"driver lock by %s%d on %s granted\r\n",
				owner_driver->name, owner_unit,
				name
		);
		free(name);
		#endif

		mtx_unlock(&driver_mtx);

		return NULL;
	}

	#if DRIVER_LOCK_DEBUG
	syslog(LOG_DEBUG,"driver %s have lock control\r\n", target_driver->name);
	#endif

	if (target_lock[lock_index(target_driver, target_unit)].owner) {
		// Target unit has a lock

		if ((target_lock[lock_index(target_driver, target_unit)].owner == owner_driver) && ((target_lock[lock_index(target_driver, target_unit)].unit == owner_unit))) {
			// Target unit is locked by owner_driver, then grant access
			#if DRIVER_LOCK_DEBUG
			syslog(LOG_DEBUG,"driver lock by %s%d on %s granted\r\n",
					owner_driver->name, owner_unit,
					name
			);
			free(name);
			#endif

			mtx_unlock(&driver_mtx);

			return NULL;
		} else {
			// Target unit is locked by another driver, then revoke access
			driver_unit_lock_error_t *error = (driver_unit_lock_error_t *)malloc(sizeof(driver_unit_lock_error_t));
			if (!error) {
				panic("not enough memory");
			}

			error->lock = &target_lock[lock_index(target_driver, target_unit)];
			error->owner_driver = owner_driver;
			error->owner_unit = owner_unit;
			error->target_driver = target_driver;
			error->target_unit = target_unit;

			#if DRIVER_LOCK_DEBUG
			syslog(LOG_DEBUG,"driver lock by %s%d on %s revoked\r\n",
					owner_driver->name, owner_unit,
					name
			);
			free(name);
			#endif

			mtx_unlock(&driver_mtx);

			driver_unlock_all(owner_driver, owner_unit);

			return error;
		}
	} else {
		// Target unit hasn't a lock, then grant access

		#if DRIVER_LOCK_DEBUG
		syslog(LOG_DEBUG,"driver lock by %s%d on %s granted\r\n",
				owner_driver->name, owner_unit,
				name
		);
		free(name);
		#endif

		target_lock[lock_index(target_driver, target_unit)].owner = owner_driver;
		target_lock[lock_index(target_driver, target_unit)].unit = owner_unit;
		target_lock[lock_index(target_driver, target_unit)].tag = tag;

		mtx_unlock(&driver_mtx);

		return NULL;
	}
}

void driver_unlock(const driver_t *owner_driver, int owner_unit, const driver_t *target_driver, int target_unit) {
	mtx_lock(&driver_mtx);

    // Get the target driver lock array
	driver_unit_lock_t *target_lock = (driver_unit_lock_t *)*target_driver->lock;

	if (target_lock) {
		target_lock[lock_index(target_driver, target_unit)].owner = NULL;
		target_lock[lock_index(target_driver, target_unit)].unit = 0;
		target_lock[lock_index(target_driver, target_unit)].tag = NULL;
	}

	mtx_unlock(&driver_mtx);
}
#endif

void _driver_init() {
    // Create driver mutex
    mtx_init(&driver_mtx, NULL, NULL, 0);

    // Init drivers
    const driver_t *cdriver = drivers;

    while (cdriver->name) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
        *cdriver->lock = NULL;
#endif
        if (cdriver->init) {
            cdriver->init();
        }
        cdriver++;
    }
}

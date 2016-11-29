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

#define LORA_DRIVER_ID 1

#define DRIVER_EXCEPTION_BASE(n) (n << 24)

typedef struct {
	const char *name;
	const int  exception_base;
	const void *error;
} driver_t;

typedef enum {
    LOCK,		// Someone needs a resource which is locked
    SETUP,      // Something fails during setup
	OPERATION   // Something fails during normal operation
} driver_error_type;

typedef struct {
    driver_error_type type;
    resource_type_t resource;
    int resource_unit;
    resource_owner_t owner;
    int owner_unit;
    int id;
    const char *msg;
    int exception;
    const driver_t *driver;
} driver_error_t;

const driver_t *driver_get(const char *name);
const char *driver_get_err_msg(driver_error_t *error);
const char *driver_get_name(driver_error_t *error);

driver_error_t *driver_lock_error(resource_lock_t *lock);
driver_error_t *driver_setup_error(const driver_t *driver, unsigned int code, const char *msg);
driver_error_t *driver_operation_error(const driver_t *driver, unsigned int code, const char *msg);

#endif

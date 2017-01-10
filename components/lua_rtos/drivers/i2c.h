/*
 * Lua RTOS, I2C driver
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

#if USE_I2C

#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#include <sys/driver.h>

#include <drivers/cpu.h>

// Internal driver structure
typedef struct i2c {
	uint8_t mode;
	uint8_t setup;
} i2c_t;

// Resources used by I2C
typedef struct {
	uint8_t sda;
	uint8_t scl;
} i2c_resources_t;

#define I2C_SLAVE	0
#define I2C_MASTER	1

// I2C errors
#define I2C_ERR_CANT_INIT                (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  1)
#define I2C_ERR_IS_NOT_SETUP             (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  2)
#define I2C_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  3)
#define I2C_ERR_INVALID_OPERATION		 (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  4)

driver_error_t *i2c_setup(int unit, int mode, int speed, int sda, int scl, int addr10_en, int addr);
driver_error_t *i2c_start(int unit);
driver_error_t *i2c_stop(int unit);

#endif /* I2C_H */

#endif

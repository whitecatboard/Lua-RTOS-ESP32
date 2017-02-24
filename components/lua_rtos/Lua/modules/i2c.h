/*
 * Lua RTOS, I2C Lua module
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

#ifndef _LI2C_H
#define	_LI2C_H

#include "luartos.h"
#include "modules.h"

#include <stdint.h>
#include <drivers/i2c.h>
#include <drivers/cpu.h>

#ifdef CPU_I2C0
#define I2C_I2C0 {LSTRKEY(CPU_I2C0_NAME), LINTVAL(CPU_I2C0)},
#else
#define I2C_I2C0
#endif

#ifdef CPU_I2C1
#define I2C_I2C1 {LSTRKEY(CPU_I2C1_NAME), LINTVAL(CPU_I2C1)},
#else
#define I2C_I2C1
#endif

#endif	


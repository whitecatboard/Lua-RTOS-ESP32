/*
 * Lua RTOS, I2C driver, bit bang implementation
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

#if LUA_USE_I2C

#ifndef I2CBB_H
#define I2CBB_H

#include <sys/drivers/i2c.h>

void i2c_bb_setup(i2c_t *unit);
void i2c_bb_idle(i2c_t *unit);
void i2c_bb_write_ack(i2c_t *unit);
void i2c_bb_write_nack(i2c_t *unit);
int  i2c_bb_read_ack(i2c_t *unit);
int  i2c_bb_write_byte(i2c_t *unit, char data);
char i2c_bb_read_byte(i2c_t *unit);
void i2c_bb_start(i2c_t *unit);
void i2c_bb_stop(i2c_t *unit);    

#endif /* I2CBB_H */

#endif

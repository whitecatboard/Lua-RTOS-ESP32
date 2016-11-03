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

#if LUA_USE_I2C

#ifndef I2C_H
#define I2C_H

#include <sys/drivers/error.h>
#include <sys/drivers/cpu.h>

#define NI2C (NI2CHW + NI2CBB)

typedef struct i2c {
    int sda;    // SDA pin
    int scl;    // SCL pin
    int delay;  // usecs delay for generate clock. Clock has a 4 * delay period
    int unit;   // Unit number
    int speed;  // Speed in hertzs
    
    // Low-Access Driver functions
    void (* i2c_setup)(struct i2c *unit);
    void (* i2c_idle)(struct i2c *unit);
    void (* i2c_write_ack)(struct i2c *unit);
    void (* i2c_write_nack)(struct i2c *unit);
    int  (* i2c_read_ack)(struct i2c *unit);
    int  (* i2c_write_byte)(struct i2c *unit, char data);
    char (* i2c_read_byte)(struct i2c *unit);
    void (* i2c_start)(struct i2c *unit);
    void (* i2c_stop)(struct i2c *unit);
} i2c_t;

tdriver_error *i2c_setup(int unit, int speed, int sda, int scl);
void i2c_start(int unit);
void i2c_stop(int unit);
int  i2c_write_address(int unit, char address, int read);
char i2c_read(int unit);
int  i2c_write(int unit, char data);

#endif /* I2C_H */

#endif

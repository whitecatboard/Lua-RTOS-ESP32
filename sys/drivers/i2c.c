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

#include "whitecat.h"

#include <unistd.h>
#include <sys/syslog.h>
#include <sys/drivers/error.h>
#include <sys/drivers/resource.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/i2c.h>
#include <sys/drivers/i2cbb.h>
#include <sys/drivers/i2chw.h>
#include <sys/drivers/gpio.h>

static i2c_t i2c[NI2C];

// Setup driver
tdriver_error *i2c_setup(int unit, int speed, int sda, int scl) {
    i2c_t *i2cu = &i2c[--unit];

    // TO DO
    // Lock resources used on sda and scl pins used
    switch (unit) {
        case 0: scl = (I2C1_PINS & 0xff00) >> 8;sda = (I2C1_PINS & 0x00ff);break;
        case 1: scl = (I2C2_PINS & 0xff00) >> 8;sda = (I2C2_PINS & 0x00ff);break;
        case 2: scl = (I2C3_PINS & 0xff00) >> 8;sda = (I2C3_PINS & 0x00ff);break;
        case 3: scl = (I2C4_PINS & 0xff00) >> 8;sda = (I2C4_PINS & 0x00ff);break;
        case 4: scl = (I2C5_PINS & 0xff00) >> 8;sda = (I2C5_PINS & 0x00ff);break;
    }

    i2cu->sda = sda;
    i2cu->scl = scl;
    i2cu->speed = speed;
    
    // Assign Low-Access Driver functions
    if (unit > (NI2CHW - 1)) {
        // Bit bang implementation
        i2cu->i2c_setup = i2c_bb_setup;
        i2cu->i2c_idle = i2c_bb_idle;
        i2cu->i2c_read_ack = i2c_bb_read_ack;
        i2cu->i2c_read_byte = i2c_bb_read_byte;
        i2cu->i2c_write_ack = i2c_bb_write_ack;
        i2cu->i2c_write_byte = i2c_bb_write_byte;
        i2cu->i2c_write_nack = i2c_bb_write_nack;
        i2cu->i2c_start = i2c_bb_start;
        i2cu->i2c_stop = i2c_bb_stop;
    } else {
        // Hardware implementation
        #ifdef I2SHW_H
        i2cu->i2c_setup = i2c_hw_setup;
        i2cu->i2c_idle = i2c_hw_idle;
        i2cu->i2c_read_ack = i2c_hw_read_ack;
        i2cu->i2c_read_byte = i2c_hw_read_byte;
        i2cu->i2c_write_ack = i2c_hw_write_ack;
        i2cu->i2c_write_byte = i2c_hw_write_byte;
        i2cu->i2c_write_nack = i2c_hw_write_nack;        
        i2cu->i2c_start = i2c_hw_start;
        i2cu->i2c_stop = i2c_hw_stop;
        #endif
    }
    
    i2cu->i2c_setup(i2cu);
    
    syslog(LOG_INFO,
        "i2c%u at pins scl=%c%d/sdc=%c%d", unit + 1,
        gpio_portname(scl), gpio_pinno(scl),
        gpio_portname(sda), gpio_pinno(sda)
    );

    return NULL;
}

// Start condition
void i2c_start(int unit) {  
    i2c_t *i2cu = &i2c[--unit];
    i2cu->i2c_start(i2cu);
}

// Stop condition
void i2c_stop(int unit) {
    i2c_t *i2cu = &i2c[--unit];

    i2cu->i2c_stop(i2cu);
}

// Write address to slave with a read / write indication
int i2c_write_address(int unit, char address, int read) {
    i2c_t *i2cu = &i2c[--unit];

    return i2cu->i2c_write_byte(i2cu, (address << 1) | read);        
}

// Read byte from slave
char i2c_read(int unit) {
    i2c_t *i2cu = &i2c[--unit];

    return i2cu->i2c_read_byte(i2cu);
}

// Write byte to slave
int i2c_write(int unit, char data) {
    i2c_t *i2cu = &i2c[--unit];

    return i2cu->i2c_write_byte(i2cu, data);
}

#endif

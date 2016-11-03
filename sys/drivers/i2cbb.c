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

#include "whitecat.h"

#include <unistd.h>
#include <sys/syslog.h>
#include <sys/delay.h>
#include <sys/drivers/error.h>
#include <sys/drivers/resource.h>
#include <sys/drivers/i2c.h>
#include <sys/drivers/i2cbb.h>
#include <sys/drivers/gpio.h>

// Setup
void i2c_bb_setup(i2c_t *unit) {
    unit->delay = (int)(1000000.0 / (3.0 * unit->speed));

    gpio_disable_analog(unit->scl);
    gpio_disable_analog(unit->sda);
    
    gpio_pin_output(unit->scl);
    gpio_pin_output(unit->sda);
    
    gpio_pin_set(unit->scl);
    gpio_pin_set(unit->sda);
}

// Wait until i2c bus is in idle state
void i2c_bb_idle(i2c_t *unit) {
    int sda = 0;
    int scl = 0;
    
    gpio_pin_input(unit->sda);

    while ((sda != 1) && (scl != 1)) {
        sda = gpio_pin_get(unit->sda);
        scl = gpio_pin_get(unit->scl);
        udelay(5);
        sda &= gpio_pin_get(unit->sda);
        scl &= gpio_pin_get(unit->scl);
        udelay(5);
    }
}

// Write ACK to slave
void i2c_bb_write_ack(i2c_t *unit) {
    // Send ACK
    udelay(unit->delay);

    gpio_pin_output(unit->sda);
    gpio_pin_clr(unit->sda);

    gpio_pin_set(unit->scl);
    udelay(unit->delay);
    udelay(unit->delay);
    
    gpio_pin_clr(unit->scl);
    udelay(unit->delay);
}

// Write NAC to slave
void i2c_bb_write_nack(i2c_t *unit) {
    // Send NACK
    udelay(unit->delay);

    gpio_pin_output(unit->sda);
    gpio_pin_set(unit->sda);

    gpio_pin_set(unit->scl);
    udelay(unit->delay);
    udelay(unit->delay);
    
    gpio_pin_clr(unit->scl);
    udelay(unit->delay);
}

// Rad ACK/NACK from slave
int i2c_bb_read_ack(i2c_t *unit) {
    int ack;
    
    // Read ACK / NACK
    gpio_pin_input(unit->sda);

    udelay(unit->delay);
    gpio_pin_set(unit->scl);
    udelay(unit->delay);

    ack = gpio_pin_get(unit->sda);    
    udelay(unit->delay);

    gpio_pin_clr(unit->scl);
    udelay(unit->delay);

    return !ack;   
}

// Write byte to slave
int i2c_bb_write_byte(i2c_t *unit, char data) {
    int i;
   
    gpio_pin_output(unit->sda);
 
    for(i=0;i < 8;i++, data = data << 1) {
        if (data & 0b10000000) {
            gpio_pin_set(unit->sda);
        } else {
            gpio_pin_clr(unit->sda);            
        }
        udelay(unit->delay);
        
        gpio_pin_set(unit->scl);
        udelay(unit->delay);
        udelay(unit->delay);

        gpio_pin_clr(unit->scl);
        udelay(unit->delay);
    }

    return i2c_bb_read_ack(unit);
}

// Read byte from slave
char i2c_bb_read_byte(i2c_t *unit) {
    int i, data;
    
    gpio_pin_input(unit->sda);
 
    for(i = 0, data = 0;i < 8;i++) {
        if (i) {
            data = data << 1;            
        }

        udelay(unit->delay);

        gpio_pin_set(unit->scl);
        udelay(unit->delay);

        if (gpio_pin_get(unit->sda)) {
            data |= 0x01;
        }     
        udelay(unit->delay);

        gpio_pin_clr(unit->scl);
        udelay(unit->delay);        
    }

    i2c_bb_write_ack(unit);

    return (char)data;
}

// Start condition
void i2c_bb_start(i2c_t *unit) {   
    gpio_pin_output(unit->sda);

    gpio_pin_set(unit->sda);
    gpio_pin_set(unit->scl);
    udelay(unit->delay);
    udelay(unit->delay);

    gpio_pin_clr(unit->sda);
    udelay(unit->delay);
    gpio_pin_clr(unit->scl);    
    udelay(unit->delay);
}

// Stop condition
void i2c_bb_stop(i2c_t *unit) {  
    gpio_pin_output(unit->sda);

    gpio_pin_set(unit->scl);
    udelay(unit->delay);
    gpio_pin_set(unit->sda);    
    udelay(unit->delay);
}

#endif

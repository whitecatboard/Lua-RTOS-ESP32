/*
 * Lua RTOS, I2C driver, hardware implementation
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
#include <sys/drivers/i2c.h>
#include <sys/drivers/i2chw.h>
#include <sys/drivers/gpio.h>
#include <machine/pic32mz.h>

// Setup
void i2c_hw_setup(i2c_t *unit) {
    // Enable I2C peripheripal
    PMD5CLR = (1 << (16 + unit->unit));
    
    // Configure module
    *(&I2C1CON + unit->unit * 0x200) = 0;    
        
    *(&I2C1CON + unit->unit * 0x200) = (1 << 15) | // Turn on module
                                 (1 << 13) | // Stop on idle mode
                                 (1 << 9);   // Slew rate control disabled
            
    // Set BRG
    *(&I2C1BRG + unit->unit * 0x200) = (unsigned int)(((((double)1.0/(((double)2.0) * ((double)(unit->speed)))) - ((double)0.00000014)) * ((double)PBCLK2_HZ)) - ((double)2.0));
}

// Wait until i2c bus is in idle state
void i2c_hw_idle(i2c_t *unit) {
    // Wait for idle state of the I2Cbus
    // (S = 0 && P = 0) || (S = 0 && P = 1)
    while (
        (((*(&I2C1STAT + unit->unit * 0x200)  & (1 << 3)) != 0) || ((*(&I2C1STAT + unit->unit * 0x200)  & (1 << 4)) != 0)) &&
        (((*(&I2C1STAT + unit->unit * 0x200)  & (1 << 3)) != 0) || ((*(&I2C1STAT + unit->unit * 0x200)  & (1 << 4)) != 1))
    ); 
}

// Write ACK to slave
void i2c_hw_write_ack(i2c_t *unit) {
    // ACK will be transmited
     *(&I2C1CONCLR + unit->unit * 0x200) = (1 << 5);
             
    // Transmit
    *(&I2C1CONSET + unit->unit * 0x200) = (1 << 4);
    
    // Waiting for done
    while (*(&I2C1CON + unit->unit * 0x200) & (1 << 4)); 
}

// Write NAC to slave
void i2c_hw_write_nack(i2c_t *unit) {
    // NACK will be transmited
    *(&I2C1CONSET + unit->unit * 0x200) = (1 << 5);

    // Transmit
    *(&I2C1CONSET + unit->unit * 0x200) = (1 << 4);
    
    // Waiting for done
    while (*(&I2C1CON + unit->unit * 0x200) & (1 << 4));  
}

// Rad ACK/NACK from slave
int i2c_hw_read_ack(i2c_t *unit) {
    // Return 1 if ACK
    return !(*(&I2C1STAT + unit->unit * 0x200) & (1 << 15));
}

// Write byte to slave
int i2c_hw_write_byte(i2c_t *unit, char data) {
    // Send data
    *(&I2C1TRN + unit->unit * 0x200) = data;

    // Waiting for done
    while (*(&I2C1STAT + unit->unit * 0x200) & (1 << 0));
 
    return i2c_hw_read_ack(unit);    
}

// Read byte from slave
char i2c_hw_read_byte(i2c_t *unit) {
    int data;
    
    // Enable receive
    *(&I2C1CONSET + unit->unit * 0x200) = (1 << 3);

    // Waiting for done
    while (*(&I2C1CON + unit->unit * 0x200) & (1 << 3));    
    
    // Waiting for data
    while (!(*(&I2C1STAT + unit->unit * 0x200) & (1 << 1)));    

    data = *(&I2C1RCV + unit->unit * 0x200);
    
    i2c_hw_write_ack(unit);
    
    return (char)data;
}

// Start condition
void i2c_hw_start(i2c_t *unit) {    
    // Set start condition
    *(&I2C1CONSET + unit->unit * 0x200) = (1 << 0);
    
    // Waiting for done
    while (*(&I2C1CON + unit->unit * 0x200) & (1 << 0)); 
}

// Stop condition
void i2c_hw_stop(i2c_t *unit) {     
    // Set stop condition
    *(&I2C1CONSET + unit->unit * 0x200) = (1 << 2);

    // Waiting for done
    while (*(&I2C1CON + unit->unit * 0x200) & (1 << 2));
}

#endif

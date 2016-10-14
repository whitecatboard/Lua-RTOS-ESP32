/*
 * Whitecat, cpu driver
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

#ifndef CPU_H
#define	CPU_H

/*
 * ----------------------------------------------------------------
 * IC2 
 * ----------------------------------------------------------------
*/

// Number of I2C units (hardware / software)
#define NI2CHW 5
#define NI2CBB 5

// PIC32MZ available hardware i2c ids
#define I2C1 1
#define I2C2 2
#define I2C3 3
#define I2C4 4
#define I2C5 5

// PIC32MZ available hardware i2c names
#define I2C1_NAME  "I2C1"
#define I2C2_NAME  "I2C2"
#define I2C3_NAME  "I2C3"
#define I2C4_NAME  "I2C4"
#define I2C5_NAME  "I2C5"

// PIC32MZ available bit bang i2c ids
#define I2CBB1 6
#define I2CBB2 7
#define I2CBB3 8
#define I2CBB4 9
#define I2CBB5 10

// PIC32MZ available bit bang i2c names
#define I2CBB1_NAME  "I2CBB1"
#define I2CBB2_NAME  "I2CBB2"
#define I2CBB3_NAME  "I2CBB3"
#define I2CBB4_NAME  "I2CBB4"
#define I2CBB5_NAME  "I2CBB5"

void cpu_init();
int cpu_revission();
void cpu_model(char *buffer);
void cpu_reset();
void cpu_show_info();
unsigned int cpu_pins();
void cpu_assign_pin(unsigned int pin, unsigned int by);
void cpu_release_pin(unsigned int pin);
unsigned int cpu_pin_assigned(unsigned int pin);
unsigned int cpu_pin_number(unsigned int pin);
unsigned int cpu_port_io_pin_mask(unsigned int port);
unsigned int cpu_port_adc_pin_mask(unsigned int port);
void cpu_sleep(int seconds);
const char *cpu_pin_name(unsigned int pin);
void _cpu_init();

#endif
/*
 * Lua RTOS, cpu driver
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

#ifndef __CPU_H__
#define	__CPU_H__

#include <stdint.h>

/*
 * ----------------------------------------------------------------
 * GPIO 
 * ----------------------------------------------------------------
*/

// PIC32MZ available GPIO pins
#define GPIO1   0x20 // PORTB
#define GPIO2   0x21
#define GPIO3   0x22
#define GPIO4   0x23
#define GPIO5   0x24
#define GPIO6   0x25
#define GPIO7   0x26
#define GPIO8   0x27
#define GPIO9   0x28
#define GPIO10  0x29
#define GPIO11  0x2A
#define GPIO12  0x2B
#define GPIO13  0x2C
#define GPIO14  0x2D
#define GPIO15  0x2E
#define GPIO16  0x2F
#define GPIO17  0x3C // PORTC
#define GPIO18  0x3D
#define GPIO19  0x3E
#define GPIO20  0x3F
#define GPIO21  0x40 // PORTD
#define GPIO22  0x41
#define GPIO23  0x42
#define GPIO24  0x43
#define GPIO25  0x44
#define GPIO26  0x45
#define GPIO27  0x49
#define GPIO28  0x4A
#define GPIO29  0x4B
#define GPIO30  0x50  // PORT E
#define GPIO31  0x51
#define GPIO32  0x52
#define GPIO33  0x53
#define GPIO34  0x54
#define GPIO35  0x55
#define GPIO36  0x56
#define GPIO37  0x57
#define GPIO38  0x60 // PORT F
#define GPIO39  0x61
#define GPIO40  0x63
#define GPIO41  0x64
#define GPIO42  0x65
#define GPIO43  0x76 // PORT G
#define GPIO44  0x77
#define GPIO45  0x78
#define GPIO46  0x79

// PIC32MZ available pin names
#define GPIO1_NAME   "RB0" // PORTA
#define GPIO2_NAME   "RB1"
#define GPIO3_NAME   "RB2"
#define GPIO4_NAME   "RB3"
#define GPIO5_NAME   "RB4"
#define GPIO6_NAME   "RB5"
#define GPIO7_NAME   "RB6"
#define GPIO8_NAME   "RB7"
#define GPIO9_NAME   "RB8"
#define GPIO10_NAME  "RB9"
#define GPIO11_NAME  "RB10"
#define GPIO12_NAME  "RB11"
#define GPIO13_NAME  "RB12"
#define GPIO14_NAME  "RB13"
#define GPIO15_NAME  "RB14"
#define GPIO16_NAME  "RB15"
#define GPIO17_NAME  "RC12" // PORTC
#define GPIO18_NAME  "RC13"
#define GPIO19_NAME  "RC14"
#define GPIO20_NAME  "RC15"
#define GPIO21_NAME  "RD0" // PORTD
#define GPIO22_NAME  "RD1"
#define GPIO23_NAME  "RD2"
#define GPIO24_NAME  "RD3"
#define GPIO25_NAME  "RD4"
#define GPIO26_NAME  "RD5"
#define GPIO27_NAME  "RD9"
#define GPIO28_NAME  "RD10"
#define GPIO29_NAME  "RD11"
#define GPIO30_NAME  "RE0"  // PORT E
#define GPIO31_NAME  "RE1"
#define GPIO32_NAME  "RE2"
#define GPIO33_NAME  "RE3"
#define GPIO34_NAME  "RE4"
#define GPIO35_NAME  "RE5"
#define GPIO36_NAME  "RE6"
#define GPIO37_NAME  "RE7"
#define GPIO38_NAME  "RF0" // PORT F
#define GPIO39_NAME  "RF1"
#define GPIO40_NAME  "RF3"	
#define GPIO41_NAME  "RF4"
#define GPIO42_NAME  "RF5"
#define GPIO43_NAME  "RG6" // PORT G
#define GPIO44_NAME  "RG7"
#define GPIO45_NAME  "RG8"
#define GPIO46_NAME  "RG9"

#define GPIO_PORTS 8
#define GPIO_PER_PORT 16

typedef uint32_t gpio_port_mask_t;

#define GPIO_ALL 0xffff

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

#endif

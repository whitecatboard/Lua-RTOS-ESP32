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

#ifndef CPU_H
#define	CPU_H

#include <stdint.h>

/*
 * ----------------------------------------------------------------
 * GPIO 
 * ----------------------------------------------------------------
*/

// ESP32 pin constants
#define PIN_GPIO36  5
#define PIN_GPIO37  6
#define PIN_GPIO38  7
#define PIN_GPIO39  8
#define PIN_GPIO34 10
#define PIN_GPIO35 11
#define PIN_GPIO32 12
#define PIN_GPIO33 13
#define PIN_GPIO25 14
#define PIN_GPIO26 15
#define PIN_GPIO27 16
#define PIN_GPIO14 17
#define PIN_GPIO12 18
#define PIN_GPIO13 20
#define PIN_GPIO15 21
#define PIN_GPIO2  22
#define PIN_GPIO0  23
#define PIN_GPIO4  24
#define PIN_GPIO16 25
#define PIN_GPIO17 27
#define PIN_GPIO9  28
#define PIN_GPIO10 29
#define PIN_GPIO11 30
#define PIN_GPIO6  31
#define PIN_GPIO7  32
#define PIN_GPIO8  33
#define PIN_GPIO5  34
#define PIN_GPIO18 35
#define PIN_GPIO23 36
#define PIN_GPIO19 38
#define PIN_GPIO22 39
#define PIN_GPIO3  40
#define PIN_GPIO1  41
#define PIN_GPIO21 42

// ESP32 available GPIO pins
#define GPIO0  0
#define GPIO1  1
#define GPIO2  2
#define GPIO3  3
#define GPIO4  4
#define GPIO5  5
#define GPIO6  6
#define GPIO7  7
#define GPIO8  8
#define GPIO9  9
#define GPIO10 10
#define GPIO11 11
#define GPIO12 12
#define GPIO13 13
#define GPIO14 14
#define GPIO15 15
#define GPIO16 16
#define GPIO17 17
#define GPIO18 18
#define GPIO19 19
#define GPIO21 21
#define GPIO22 22
#define GPIO23 23
#define GPIO25 25
#define GPIO26 26
#define GPIO27 27
#define GPIO32 32
#define GPIO33 33
#define GPIO34 34
#define GPIO35 35
#define GPIO36 36
#define GPIO37 37
#define GPIO38 38
#define GPIO39 39

// ESP32 available pin names
#define GPIO0_NAME  "GPIO0"
#define GPIO1_NAME  "GPIO1"
#define GPIO2_NAME  "GPIO2"
#define GPIO3_NAME  "GPIO3"
#define GPIO4_NAME  "GPIO4"
#define GPIO5_NAME  "GPIO5"
#define GPIO6_NAME  "GPIO6"
#define GPIO7_NAME  "GPIO7"
#define GPIO8_NAME  "GPIO8"
#define GPIO9_NAME  "GPIO9"
#define GPIO10_NAME "GPIO10"
#define GPIO11_NAME "GPIO11"
#define GPIO12_NAME "GPIO12"
#define GPIO13_NAME "GPIO13"
#define GPIO14_NAME "GPIO14"
#define GPIO15_NAME "GPIO15"
#define GPIO16_NAME "GPIO16"
#define GPIO17_NAME "GPIO17"
#define GPIO18_NAME "GPIO18"
#define GPIO19_NAME "GPIO19"
#define GPIO21_NAME "GPIO21"
#define GPIO22_NAME "GPIO22"
#define GPIO23_NAME "GPIO23"
#define GPIO25_NAME "GPIO25"
#define GPIO26_NAME "GPIO26"
#define GPIO27_NAME "GPIO27"
#define GPIO32_NAME "GPIO32"
#define GPIO33_NAME "GPIO33"
#define GPIO34_NAME "GPIO34"
#define GPIO35_NAME "GPIO35"
#define GPIO36_NAME "GPIO36"
#define GPIO37_NAME "GPIO37"
#define GPIO38_NAME "GPIO38"
#define GPIO39_NAME "GPIO39"

// ESP32 has only 1 GPIO port
#define GPIO_PORTS 1

// ESP32 has 16 GPIO per port
#define GPIO_PER_PORT 39

// ESP32 needs 64 bits for port mask
typedef uint64_t gpio_port_mask_t;
#define GPIO_ALL 0b111111110000111011101111111111111111111UL

/*
 * ----------------------------------------------------------------
 * IC2 
 * ----------------------------------------------------------------
*/

// Number of I2C units (hardware / software)
#define NI2CHW 0
#define NI2CBB 1

// ESP32 available bit bang i2c ids
#define I2CBB1 1

// ESP32 available bit bang i2c names
#define I2CBB1_NAME  "I2CBB1"

#endif

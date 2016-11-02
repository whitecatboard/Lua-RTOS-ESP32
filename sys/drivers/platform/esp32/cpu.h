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
#define PIN_GPIO02 22
#define PIN_GPIO00 23
#define PIN_GPIO04 24
#define PIN_GPIO16 25
#define PIN_GPIO17 27
#define PIN_GPIO09 28
#define PIN_GPIO10 29
#define PIN_GPIO11 30
#define PIN_GPIO06 31
#define PIN_GPIO07 32
#define PIN_GPIO08 33
#define PIN_GPIO05 34
#define PIN_GPIO18 35
#define PIN_GPIO23 36
#define PIN_GPIO19 38
#define PIN_GPIO22 39
#define PIN_GPIO03 40
#define PIN_GPIO01 41
#define PIN_GPIO21 42

// ESP32 available GPIO pins
#define GPIO00 0
#define GPIO01 1
#define GPIO02 2
#define GPIO03 3
#define GPIO04 4
#define GPIO05 5
#define GPIO06 6
#define GPIO07 7
#define GPIO08 8
#define GPIO09 9
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
#define GPIO00_NAME "GPIO00"
#define GPIO01_NAME "GPIO01"
#define GPIO02_NAME "GPIO02"
#define GPIO03_NAME "GPIO03"
#define GPIO04_NAME "GPIO04"
#define GPIO05_NAME "GPIO05"
#define GPIO06_NAME "GPIO06"
#define GPIO07_NAME "GPIO07"
#define GPIO08_NAME "GPIO08"
#define GPIO09_NAME "GPIO09"
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
unsigned int cpu_port_number(unsigned int pin);
unsigned int cpu_port_io_pin_mask(unsigned int port);
unsigned int cpu_port_adc_pin_mask(unsigned int port);
void cpu_idle(int seconds);
const char *cpu_pin_name(unsigned int pin);
unsigned int cpu_has_gpio(unsigned int port, unsigned int pin);
unsigned int cpu_has_port(unsigned int port);
void cpu_sleep(unsigned int seconds);
int cpu_reset_reason();
void cpu_sleep();
#endif
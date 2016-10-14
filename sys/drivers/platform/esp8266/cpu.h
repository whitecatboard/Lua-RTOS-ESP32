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

// ESP8266 pin constants
#define PIN_TOUT   5
#define PIN_GPIO16 7
#define PIN_GPIO14 8
#define PIN_GPIO12 9
#define PIN_GPIO13 11
#define PIN_GPIO15 12
#define PIN_GPIO2  13
#define PIN_GPIO0  14
#define PIN_GPIO4  15
#define PIN_SD_D2  17
#define PIN_SD_D3  18
#define PIN_SD_CMD 19
#define PIN_SD_CLK 20
#define PIN_SD_D0  21
#define PIN_SD_D1  22
#define PIN_GPIO5  23
#define PIN_GPIO3  24
#define PIN_GPIO1  25
#define PIN_NOPIN  32

// ESP8266 available GPIO pins
#define GPIO1  1
#define GPIO2  2
#define GPIO3  3
#define GPIO4  4
#define GPIO5  5
#define GPIO12 12
#define GPIO13 13
#define GPIO14 14
#define GPIO15 15
#define GPIO16 16

// ESP8266 available pin names
#define GPIO1_NAME  "GPIO1"
#define GPIO2_NAME  "GPIO2"
#define GPIO3_NAME  "GPIO3"
#define GPIO4_NAME  "GPIO4"
#define GPIO5_NAME  "GPIO5"
#define GPIO12_NAME "GPIO12"
#define GPIO13_NAME "GPIO13"
#define GPIO14_NAME "GPIO14"
#define GPIO15_NAME "GPIO15"
#define GPIO16_NAME "GPIO16"

// ESP8266 has only 1 GPIO port
#define GPIO_PORTS 1

// ESP8266 has 16 GPIO per port
#define GPIO_PER_PORT 16

#define GPIO_ALL 0b11111000000111110

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
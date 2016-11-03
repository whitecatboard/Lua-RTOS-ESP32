/*
 * Lua RTOS, gpio driver
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

#ifndef __ROOT_GPIO_H__
#define __ROOT_GPIO_H__

#include <sys/drivers/cpu.h>

// Include platform depend definitions
#ifdef PLATFORM_ESP8266
#include <sys/drivers/platform/esp8266/gpio.h>
#endif

#ifdef PLATFORM_ESP32
#include <sys/drivers/platform/esp32/gpio.h>
#endif

#ifdef PLATFORM_PIC32MZ
#include <sys/drivers/platform/pic32mz/gpio.h>
#endif

void gpio_pin_input_mask(unsigned int port, gpio_port_mask_t pinmask);
void gpio_pin_output_mask(unsigned int port, gpio_port_mask_t pinmask);
void gpio_pin_set_mask(unsigned int port, gpio_port_mask_t pinmask);
void gpio_pin_clr_mask(unsigned int port, gpio_port_mask_t pinmask);
gpio_port_mask_t gpio_pin_get_mask(unsigned int port, gpio_port_mask_t pinmask);
void gpio_pin_pullup_mask(unsigned int port, gpio_port_mask_t pinmask);
void gpio_pin_pulldwn_mask(unsigned int port, gpio_port_mask_t pinmask);
void gpio_pin_nopull_mask(unsigned int port, gpio_port_mask_t pinmask);
void gpio_port_input(unsigned int port);
void gpio_port_output(unsigned int port);
void gpio_port_set(unsigned int port, gpio_port_mask_t mask);
gpio_port_mask_t gpio_port_get(unsigned int port);
char gpio_portname(int pin);
unsigned int cpu_gpio_number(unsigned int pin);
int gpio_pinno(int pin);
void gpio_disable_analog(int pin);

#endif

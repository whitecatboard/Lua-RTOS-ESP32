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

#ifndef __GPIO_H__
#define __GPIO_H__

#include <driver/gpio.h>
#include <rom/gpio.h>

#include "drivers/gpio.h"
#include "drivers/cpu.h"

// GPIO errors
#define GPIO_ERR_CANT_INIT                (DRIVER_EXCEPTION_BASE(GPIO_DRIVER_ID) |  0)

#define ETS_GPIO_INUM 4

#define gpio_pin_input(gpio) gpio_pad_select_gpio(gpio);gpio_set_direction(gpio, GPIO_MODE_INPUT)
#define gpio_pin_output(gpio) gpio_pad_select_gpio(gpio);gpio_set_direction(gpio, GPIO_MODE_OUTPUT)
#define gpio_pin_opendrain(gpio) gpio_pad_select_gpio(gpio);gpio_set_direction(gpio, GPIO_MODE_INPUT_OUTPUT_OD)

#define gpio_pin_set(gpio) gpio_set_level(gpio, 1)
#define gpio_pin_clr(gpio) gpio_set_level(gpio, 0)

#define gpio_pin_inv(gpio) \
if (gpio < 32) { \
	if (GPIO.out & (1 << gpio)) {gpio_pin_clr(gpio);} else {gpio_pin_set(gpio);} \
} else { \
	if (GPIO.out1.val & (1 << gpio)) {gpio_pin_clr(gpio);} else {gpio_pin_set(gpio);} \
}

#define gpio_pin_get(gpio) gpio_get_level(gpio)


#define gpio_pin_pullup(gpio) gpio_set_pull_mode(gpio, GPIO_PULLUP_ONLY)
#define gpio_pin_pulldwn(gpio) gpio_set_pull_mode(gpio, GPIO_PULLDOWN_ONLY)
#define gpio_pin_nopull(gpio) gpio_set_pull_mode(gpio, GPIO_FLOATING)


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
const char *gpio_portname(int pin);
unsigned int cpu_gpio_number(unsigned int pin);
int gpio_name(int pin);
void gpio_disable_analog(int pin);

void _gpio_init();

#endif

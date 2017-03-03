/*
 * Lua RTOS, gpio driver
 *
 * Copyright (C) 2015 - 2017
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

#include <sys/driver.h>

#include <driver/gpio.h>
#include <rom/gpio.h>

#include "drivers/gpio.h"
#include "drivers/cpu.h"

// GPIO errors
#define GPIO_ERR_INVALID_PIN_DIRECTION        (DRIVER_EXCEPTION_BASE(GPIO_DRIVER_ID) |  0)
#define GPIO_ERR_INVALID_PIN                  (DRIVER_EXCEPTION_BASE(GPIO_DRIVER_ID) |  1)
#define GPIO_ERR_INVALID_PORT                 (DRIVER_EXCEPTION_BASE(GPIO_DRIVER_ID) |  2)

void gpio_ll_pin_set(uint8_t pin);
void gpio_ll_pin_clr(uint8_t pin);
void gpio_ll_pin_inv(int8_t pin);

driver_error_t *gpio_pin_output(uint8_t pin);
driver_error_t *gpio_pin_input(uint8_t pin);
driver_error_t *gpio_pin_set(uint8_t pin);
driver_error_t *gpio_pin_clr(uint8_t pin);
driver_error_t *gpio_pin_inv(uint8_t pin);
driver_error_t *gpio_pin_get(uint8_t pin, uint8_t *val);
driver_error_t *gpio_pin_input_mask(uint8_t port, gpio_pin_mask_t pinmask);
driver_error_t *gpio_pin_output_mask(uint8_t port, gpio_pin_mask_t pinmask);
driver_error_t *gpio_pin_set_mask(uint8_t port, gpio_pin_mask_t pinmask);
driver_error_t *gpio_pin_clr_mask(uint8_t port, gpio_pin_mask_t pinmask);
driver_error_t *gpio_pin_get_mask(uint8_t port, gpio_pin_mask_t pinmask, gpio_pin_mask_t *value);
driver_error_t *gpio_pin_pullup_mask(uint8_t port, gpio_pin_mask_t pinmask);
driver_error_t *gpio_pin_pulldwn_mask(uint8_t port, gpio_pin_mask_t pinmask);
driver_error_t *gpio_pin_nopull_mask(uint8_t port, gpio_pin_mask_t pinmask);
driver_error_t *gpio_port_input(uint8_t port);
driver_error_t *gpio_port_output(uint8_t port);
driver_error_t *gpio_port_set(uint8_t port, gpio_pin_mask_t mask);
driver_error_t *gpio_port_get(uint8_t port, gpio_pin_mask_t *value);

const char *gpio_portname(uint8_t pin);
uint8_t gpio_name(uint8_t pin);

#endif

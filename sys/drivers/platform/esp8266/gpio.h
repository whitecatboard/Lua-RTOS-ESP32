/*
 * Whitecat, gpio driver
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

#include "espressif/esp_common.h"
#include "esp8266.h"
#include "esp/iomux.h"

#define gpio_pin_input(gpio)  GPIO.ENABLE_OUT_CLEAR = BIT(gpio);iomux_set_gpio_function(gpio, false)
#define gpio_pin_output(gpio) GPIO.CONF[gpio] &= ~GPIO_CONF_OPEN_DRAIN;GPIO.ENABLE_OUT_SET = BIT(gpio);iomux_set_gpio_function(gpio, false)
#define gpio_pin_opendrain(gpio) GPIO.CONF[gpio] |= GPIO_CONF_OPEN_DRAIN;GPIO.ENABLE_OUT_SET = BIT(gpio);iomux_set_gpio_function(gpio, true)

#define gpio_pin_set(gpio) GPIO.OUT_SET = BIT(gpio)
#define gpio_pin_clr(gpio) GPIO.OUT_CLEAR = BIT(gpio)
#define gpio_pin_inv(gpio) if (GPIO.OUT & BIT(gpio)) {gpio_pin_clr(gpio);} else {gpio_pin_set(gpio);}
#define gpio_pin_get(gpio) ((GPIO.IN & BIT(gpio))?1:0)


#define gpio_pin_pullup(gpio) iomux_set_pullup_flags(gpio_to_iomux(gpio), IOMUX_PIN_PULLUP)
#define gpio_pin_pulldwn(gpio) iomux_set_pullup_flags(gpio_to_iomux(gpio), IOMUX_PIN_PULLDOWN)
#define gpio_pin_nopull(gpio) iomux_set_pullup_flags(gpio_to_iomux(gpio), 0)

/*
void gpio_enable_analog(int pin);
void gpio_disable_analog(int pin);

char gpio_portname(int pin);
int gpio_pinno(int pin);
int gpio_is_io_port(int port);
int gpio_is_io_port_pin(int pin);
int gpio_port_has_analog(int port);
*/

void gpio_pin_input_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_output_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_set_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_clr_mask(unsigned int port, unsigned int pinmask);
unsigned int gpio_pin_get_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_pullup_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_pulldwn_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_nopull_mask(unsigned int port, unsigned int pinmask);
void gpio_port_input(unsigned int port);
void gpio_port_output(unsigned int port);
void gpio_port_set(unsigned int port, unsigned int mask);
unsigned int gpio_port_get(unsigned int port);

#endif
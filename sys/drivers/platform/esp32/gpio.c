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

#include "gpio.h"

#include <sys/drivers/cpu.h>

// Configure gpio as input using a mask
// If bit n on mask is set to 1 the gpio is configured
void gpio_pin_input_mask(unsigned int port, gpio_port_mask_t pinmask) {
	gpio_port_mask_t mask = 1;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_input(i);
		}
		
		mask = (mask << 1);
	}	
}

// Configure all gpio's port as input
void gpio_port_input(unsigned int port) {
	gpio_pin_input_mask(port, GPIO_ALL);
}

// Configure gpio as output using a mask
// If bit n on mask is set to 1 the gpio is configured
void gpio_pin_output_mask(unsigned int port, gpio_port_mask_t pinmask) {
	gpio_port_mask_t mask = 1;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_output(i);
		}
		
		mask = (mask << 1);
	}	
}

// Configure all gpio's port as output
void gpio_port_output(unsigned int port) {
	gpio_pin_output_mask(port, GPIO_ALL);
}

// Set gpio pull-up using a mask
// If bit n on mask is set to 1 the gpio is set pull-up
void gpio_pin_pullup_mask(unsigned int port, gpio_port_mask_t pinmask) {
	gpio_port_mask_t mask = 1;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_pullup(i);
		}
		
		mask = (mask << 1);
	}	
}

// Set gpio pull-down using a mask
// If bit n on mask is set to 1 the gpio is set pull-up
void gpio_pin_pulldwn_mask(unsigned int port, gpio_port_mask_t pinmask) {
	gpio_port_mask_t mask = 1;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_pulldwn(i);
		}
		
		mask = (mask << 1);
	}	
}

// Set gpio with no pull-up and no pull-down using a mask
// If bit n on mask is set to 1 the gpio with no pull-up and no pull-down
void gpio_pin_nopull_mask(unsigned int port, gpio_port_mask_t pinmask) {
	gpio_port_mask_t mask = 1;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_nopull(i);
		}
		
		mask = (mask << 1);
	}	
}

// Put gpio on the high state using a mask
// If bit n on mask is set to 1 the gpio is put on the high state
void gpio_pin_set_mask(unsigned int port, gpio_port_mask_t pinmask) {
	gpio_port_mask_t mask = 1;
	int i;

	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_set(i);
		}

		mask = (mask << 1);
	}
}

// Put port gpio's on the high state
// If bit n on mask is set to 1 the gpio is put on the high state
void gpio_port_set(unsigned int port, gpio_port_mask_t pinmask) {
	gpio_port_mask_t mask = 1;
	int i;

	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_set(i);
		}

		mask = (mask << 1);
	}
}

// Put gpio on the low state using a mask
// If bit n on mask is set to 1 the gpio is put on the low state
void gpio_pin_clr_mask(unsigned int port, gpio_port_mask_t pinmask) {
	gpio_port_mask_t mask = 1;
	int i;

	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_clr(i);
		}

		mask = (mask << 1);
	}
}

// Get gpio values using a mask
gpio_port_mask_t gpio_pin_get_mask(unsigned int port, gpio_port_mask_t pinmask) {
	gpio_port_mask_t mask = 1;
	gpio_port_mask_t get_mask = 0;
	int i;

	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			get_mask |= (gpio_pin_get(i) << i);
		}

		mask = (mask << 1);
	}

	return get_mask;
}

// Get port gpio values
gpio_port_mask_t gpio_port_get(unsigned int port) {
	return gpio_pin_get_mask(port, GPIO_ALL);
}

// Get all port gpio values
gpio_port_mask_t gpio_port_get_mask(unsigned int port) {
	return gpio_pin_get_mask(port, GPIO_ALL);
}

char gpio_portname(int pin) {
    return '0';
}

int gpio_pinno(int pin) {
    return pin;
}
void gpio_disable_analog(int pin) {
	
}

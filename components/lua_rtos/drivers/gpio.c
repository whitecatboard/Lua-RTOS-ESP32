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

#include <string.h>

#include <sys/driver.h>
#include <drivers/gpio.h>
#include <drivers/cpu.h>

// This macro gets a reference for this driver into drivers array
#define GPIO_DRIVER driver_get_by_name("gpio")

// Driver locks
driver_unit_lock_t gpio_locks[CPU_LAST_GPIO];

// Driver errors
DRIVER_REGISTER_ERROR(GPIO, gpio, CannotSetup, "can't setup", GPIO_ERR_CANT_INIT);

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

const char *gpio_portname(int pin) {
    return "GPIO";
}

int gpio_name(int pin) {
	switch (pin) {
		case GPIO36: return 36;
		case GPIO39: return 39;
		case GPIO34: return 34;
		case GPIO35: return 35;
		case GPIO32: return 32;
		case GPIO33: return 33;
		case GPIO25: return 25;
		case GPIO26: return 26;
		case GPIO27: return 27;
		case GPIO14: return 14;
		case GPIO12: return 12;
		case GPIO13: return 13;
		case GPIO9:  return 9;
		case GPIO10: return 10;
		case GPIO11: return 11;
		case GPIO6:  return 6;
		case GPIO7:  return 7;
		case GPIO8:  return 8;
		case GPIO15: return 15;
		case GPIO2:  return 2;
		case GPIO0:  return 0;
		case GPIO4:  return 4;
		case GPIO16: return 16;
		case GPIO17: return 17;
		case GPIO5:  return 5;
		case GPIO18: return 18;
		case GPIO19: return 19;
		case GPIO21: return 21;
		case GPIO3:  return 3;
		case GPIO1:  return 1;
		case GPIO22: return 22;
		case GPIO23: return 23;
	}

    return -1;
}
void gpio_disable_analog(int pin) {
	
}

void _gpio_init() {
	// Init lock array
	memset(gpio_locks, 0, sizeof(gpio_locks));
}

DRIVER_REGISTER(GPIO,gpio,gpio_locks,_gpio_init,NULL);


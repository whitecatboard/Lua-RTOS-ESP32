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

#include "esp_err.h"
#include "esp_attr.h"

#include <string.h>

#include <sys/driver.h>
#include <drivers/gpio.h>
#include <drivers/cpu.h>

// Driver locks
static driver_unit_lock_t gpio_locks[CPU_LAST_GPIO + 1];

// Driver errors
DRIVER_REGISTER_ERROR(GPIO, gpio, InvalidPinDirection, "invalid pin direction", GPIO_ERR_INVALID_PIN_DIRECTION);
DRIVER_REGISTER_ERROR(GPIO, gpio, InvalidPin, "invalid pin", GPIO_ERR_INVALID_PIN);
DRIVER_REGISTER_ERROR(GPIO, gpio, InvalidPort, "invalid port", GPIO_ERR_INVALID_PORT);

/*
 * Low level gpio operations
 */
void IRAM_ATTR gpio_ll_pin_set(uint8_t pin) {
	if (pin < 32) {
		GPIO.out_w1ts = (1 << pin);
	} else {
		GPIO.out1_w1ts.data = (1 << (pin - 32));
	}
}

void IRAM_ATTR gpio_ll_pin_clr(uint8_t pin) {
	if (pin < 32) {
		GPIO.out_w1tc = (1 << pin);
	} else {
		GPIO.out1_w1tc.data = (1 << (pin - 32));
	}
}

void IRAM_ATTR gpio_ll_pin_inv(int8_t pin) {
	if (pin < 32) {
		if (GPIO.out & (1 << pin)) {
			gpio_ll_pin_clr(pin);
		} else {
			gpio_ll_pin_set(pin);
		}
	} else {
		if (GPIO.out1.val & (1 << pin)) {
			gpio_ll_pin_clr(pin);
		} else {
			gpio_ll_pin_set(pin);
		}
	}
}

/*
 * Operations over a single pin
 *
 */
driver_error_t *gpio_pin_output(uint8_t pin) {
	gpio_config_t io_conf;

	// Sanity checks
	if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << pin))) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
	}

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (GPIO_BIT_MASK << pin);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;

    gpio_config(&io_conf);

    return NULL;
}

driver_error_t *gpio_pin_input(uint8_t pin) {
	gpio_config_t io_conf;

	// Sanity checks
	if (!(GPIO_ALL_IN & (GPIO_BIT_MASK << pin))) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
	}

	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (GPIO_BIT_MASK << pin);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;

    gpio_config(&io_conf);

    return NULL;
}

driver_error_t *gpio_pin_set(uint8_t pin) {
	// Sanity checks
	if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << pin))) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
	}

	gpio_ll_pin_set(pin);

	return NULL;
}

driver_error_t *gpio_pin_clr(uint8_t pin) {
	// Sanity checks
	if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << pin))) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
	}

	gpio_ll_pin_clr(pin);

	return NULL;
}

driver_error_t *gpio_pin_inv(uint8_t pin) {
	driver_error_t *error = NULL;

	// Sanity checks
	if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << pin))) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
	}

	gpio_ll_pin_inv(pin);

	return error;
}

driver_error_t *gpio_pin_get(uint8_t pin, uint8_t *val) {
	// Sanity checks
	if (!(GPIO_ALL_IN & (GPIO_BIT_MASK << pin))) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
	}

	*val = gpio_get_level(pin);

	return NULL;
}

driver_error_t *gpio_pin_pullup(uint8_t pin) {
	// Sanity checks
	if (!(GPIO_ALL & (GPIO_BIT_MASK << pin))) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
	}

	gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);

	return NULL;
}

driver_error_t *gpio_pin_pulldwn(uint8_t pin) {
	// Sanity checks
	if (!(GPIO_ALL & (GPIO_BIT_MASK << pin))) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
	}

	gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);

	return NULL;
}

driver_error_t *gpio_pin_nopull(uint8_t pin) {
	// Sanity checks
	if (!(GPIO_ALL & (GPIO_BIT_MASK << pin))) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
	}

	gpio_set_pull_mode(pin, GPIO_FLOATING);

	return NULL;
}

/*
 * Operations over port pins
 *
 */

// Configure gpio as input using a mask
// If bit n on mask is set to 1 the gpio is configured
driver_error_t *gpio_pin_input_mask(uint8_t port, gpio_pin_mask_t pinmask) {
	gpio_config_t io_conf;

	// Sanity checks
	if (!(GPIO_PORT_ALL & port)) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
	}

	if (!(GPIO_ALL_IN & pinmask)) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
	}

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = pinmask;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;

    gpio_config(&io_conf);

    return NULL;
}

// Configure gpio as output using a mask
// If bit n on mask is set to 1 the gpio is configured
driver_error_t *gpio_pin_output_mask(uint8_t port, gpio_pin_mask_t pinmask) {
	gpio_config_t io_conf;

	// Sanity checks
	if (!(GPIO_ALL_OUT & pinmask)) {
		return driver_operation_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
	}

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = pinmask;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;

    gpio_config(&io_conf);

    return NULL;
}

// Set gpio pull-up using a mask
// If bit n on mask is set to 1 the gpio is set pull-up
driver_error_t *gpio_pin_pullup_mask(uint8_t port, gpio_pin_mask_t pinmask) {
	driver_error_t *error = NULL;
	gpio_pin_mask_t mask = GPIO_BIT_MASK;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			if ((error = gpio_pin_pullup(i))) {
				return error;
			}
		}
		
		mask = (mask << 1);
	}	

	return NULL;
}

// Set gpio pull-down using a mask
// If bit n on mask is set to 1 the gpio is set pull-up
driver_error_t *gpio_pin_pulldwn_mask(uint8_t port, gpio_pin_mask_t pinmask) {
	driver_error_t *error = NULL;
	gpio_pin_mask_t mask = GPIO_BIT_MASK;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			if ((error = gpio_pin_pulldwn(i))) {
				return error;
			}
		}
		
		mask = (mask << 1);
	}	

	return NULL;
}

// Set gpio with no pull-up and no pull-down using a mask
// If bit n on mask is set to 1 the gpio with no pull-up and no pull-down
driver_error_t *gpio_pin_nopull_mask(uint8_t port, gpio_pin_mask_t pinmask) {
	driver_error_t *error = NULL;
	gpio_pin_mask_t mask = GPIO_BIT_MASK;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			if ((error = gpio_pin_nopull(i))) {
				return error;
			}
		}
		
		mask = (mask << 1);
	}	

	return NULL;
}

// Put gpio on the high state using a mask
// If bit n on mask is set to 1 the gpio is put on the high state
driver_error_t *gpio_pin_set_mask(uint8_t port, gpio_pin_mask_t pinmask) {
	driver_error_t *error = NULL;
	gpio_pin_mask_t mask = GPIO_BIT_MASK;
	int i;

	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			if ((error = gpio_pin_set(i))) {
				return error;
			}
		}

		mask = (mask << 1);
	}

	return NULL;
}

// Put port gpio's on the high state
// If bit n on mask is set to 1 the gpio is put on the high state
driver_error_t *gpio_port_set(uint8_t port, gpio_pin_mask_t pinmask) {
	driver_error_t *error = NULL;
	gpio_pin_mask_t mask = GPIO_BIT_MASK;
	int i;

	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			if ((error = gpio_pin_set(i))) {
				return error;
			}
		}

		mask = (mask << 1);
	}

	return error;
}

// Put gpio on the low state using a mask
// If bit n on mask is set to 1 the gpio is put on the low state
driver_error_t *gpio_pin_clr_mask(uint8_t port, gpio_pin_mask_t pinmask) {
	driver_error_t *error = NULL;
	gpio_pin_mask_t mask = GPIO_BIT_MASK;
	int i;

	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			if ((error = gpio_pin_clr(i))) {
				return error;
			}
		}

		mask = (mask << 1);
	}

	return NULL;
}

// Get gpio values using a mask
driver_error_t *gpio_pin_get_mask(uint8_t port, gpio_pin_mask_t pinmask, gpio_pin_mask_t *value) {
	driver_error_t *error = NULL;
	gpio_pin_mask_t mask = GPIO_BIT_MASK;
	gpio_pin_mask_t get_mask = 0;
	uint8_t val;
	int i;

	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			if ((error = gpio_pin_get(i, &val))) {
				return error;
			};

			get_mask |= ((gpio_pin_mask_t)val << i);
		}

		mask = (mask << 1);
	}

	*value = get_mask;

	return NULL;
}

/*
 * Operations over all port pins
 *
 */

// Configure all gpio's port as input
driver_error_t *gpio_port_input(uint8_t port) {
	return gpio_pin_input_mask(port, GPIO_ALL_IN);
}

// Configure all gpio's port as output
driver_error_t *gpio_port_output(uint8_t port) {
	return gpio_pin_output_mask(port, GPIO_ALL_OUT);
}

// Get port gpio values
driver_error_t *gpio_port_get(uint8_t port, gpio_pin_mask_t *value) {
	return gpio_pin_get_mask(port, GPIO_ALL_IN, value);
}

// Get all port gpio values
driver_error_t *gpio_port_get_mask(uint8_t port, gpio_pin_mask_t *value) {
	return gpio_pin_get_mask(port, GPIO_ALL_IN, value);
}

/*
 * Information operations
 *
 */
const char *gpio_portname(uint8_t pin) {
    return "GPIO";
}

uint8_t gpio_name(uint8_t pin) {
	return pin;
}

DRIVER_REGISTER(GPIO,gpio,gpio_locks,NULL,NULL);


/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS, gpio driver
 *
 */

#ifndef __GPIO_H__
#define __GPIO_H__

#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

#include <sys/driver.h>

#include <driver/gpio.h>
#include <rom/gpio.h>

#include "drivers/gpio.h"
#include "drivers/cpu.h"

// GPIO errors
#define GPIO_ERR_INVALID_PIN_DIRECTION        (DRIVER_EXCEPTION_BASE(GPIO_DRIVER_ID) |  0)
#define GPIO_ERR_INVALID_PIN                  (DRIVER_EXCEPTION_BASE(GPIO_DRIVER_ID) |  1)
#define GPIO_ERR_INVALID_PORT                 (DRIVER_EXCEPTION_BASE(GPIO_DRIVER_ID) |  2)
#define GPIO_ERR_NOT_ENOUGH_MEMORY			  (DRIVER_EXCEPTION_BASE(GPIO_DRIVER_ID) |  3)
#define GPIO_ERR_PULL_UP_NOT_ALLOWED		  (DRIVER_EXCEPTION_BASE(GPIO_DRIVER_ID) |  4)
#define GPIO_ERR_PULL_DOWN_NOT_ALLOWED		  (DRIVER_EXCEPTION_BASE(GPIO_DRIVER_ID) |  5)
#define GPIO_ERR_INT_NOT_ALLOWED			  (DRIVER_EXCEPTION_BASE(GPIO_DRIVER_ID) |  6)

extern const int gpio_errors;
extern const int gpio_error_map;

driver_error_t *gpio_ll_pin_set(uint8_t pin);
driver_error_t *gpio_ll_pin_clr(uint8_t pin);
driver_error_t *gpio_ll_pin_inv(int8_t pin);
uint8_t IRAM_ATTR gpio_ll_pin_get(int8_t pin);

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
driver_error_t *gpio_pin_pullup(uint8_t pin);
driver_error_t *gpio_pin_pulldwn(uint8_t pin);
driver_error_t *gpio_pin_nopull(uint8_t pin);
driver_error_t *gpio_pin_inv_mask(uint8_t port, gpio_pin_mask_t pinmask);
driver_error_t *gpio_isr_attach(uint8_t pin, gpio_isr_t gpio_isr, gpio_int_type_t type, void *args);
driver_error_t *gpio_isr_detach(uint8_t pin);
uint8_t gpio_is_input(uint8_t pin);
uint8_t gpio_is_output(uint8_t pin);
int gpio_get_pulse_time(uint8_t pin, uint8_t level, uint32_t usecs);

const char *gpio_portname(uint8_t pin);
uint8_t gpio_name(uint8_t pin);

#define GPIO_CHECK_INPUT(gpio) \
	(gpio < 40 \
		?(GPIO_ALL_IN & (GPIO_BIT_MASK << gpio)) \
		:(EXTERNAL_GPIO \
			?(gpio < 80) \
			:0 \
		) \
	)

#define GPIO_CHECK_OUTPUT(gpio) \
	(gpio < 40 \
		?(GPIO_ALL_OUT & (GPIO_BIT_MASK << gpio)) \
		:(EXTERNAL_GPIO \
			?(gpio < 80) \
			:0 \
		) \
	)

#define gpio_ignore_error(e,s) if (((e) = (s)) != NULL) free(e);

#endif

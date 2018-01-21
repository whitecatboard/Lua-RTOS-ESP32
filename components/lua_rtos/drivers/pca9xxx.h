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
 * Lua RTOS, pca9xxx driver
 *
 */

#ifndef PCA9698_H
#define	PCA9698_H

#include "luartos.h"

#if EXTERNAL_GPIO

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_attr.h"

#include <driver/gpio.h>

#include <stdint.h>

#include <sys/driver.h>

#if CONFIG_GPIO_PCA9698 ||  CONFIG_GPIO_PCA9505
	// Number of banks
	#define PCA9xxx_BANKS 5

	// Number of pins
	#define PCA9xxx_PINS 40
#endif

// Convert a GPIO number to it's bank number
#define PCA9xxx_GPIO_BANK_NUM(gpio) (gpio >> 3)

// Convert a GPIO number to it's position into it's bank
#define PCA9xxx_GPIO_BANK_POS(gpio) (gpio % 8)

typedef struct {
	uint8_t direction[PCA9xxx_BANKS];
	uint8_t latch[PCA9xxx_BANKS];
	gpio_isr_t isr_func[PCA9xxx_PINS];
	uint8_t isr_type[PCA9xxx_PINS];
	void *isr_args[PCA9xxx_PINS];
	TaskHandle_t task;
	SemaphoreHandle_t mtx;
	int i2cdevice;
} pca_9xxx_t;

// PCA9698 errors
#define PCA9xxx_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(PCA9xxx_DRIVER_ID) |  0)

extern const int pca9xxx_erros;
extern const int pca9xxx_error_map;

driver_error_t *pca_9xxx_setup();
driver_error_t *pca_9xxx_pin_output(uint8_t pin);
driver_error_t *pca_9xxx_pin_input(uint8_t pin);
driver_error_t *pca_9xxx_pin_set(uint8_t pin);
driver_error_t *pca_9xxx_pin_clr(uint8_t pin);
driver_error_t *pca_9xxx_pin_inv(uint8_t pin);
uint8_t pca_9xxx_pin_get(uint8_t pin);
driver_error_t *pca_9xxx_pin_input_mask(uint8_t port, uint8_t pinmask);
driver_error_t *pca_9xxx_pin_output_mask(uint8_t port, uint8_t pinmask);
driver_error_t *pca_9xxx_pin_set_mask(uint8_t port, uint8_t pinmask);
driver_error_t *pca_9xxx_pin_clr_mask(uint8_t port, uint8_t pinmask);
driver_error_t *pca_9xxx_pin_inv_mask(uint8_t port, uint8_t pinmask);
void pca_9xxx_pin_get_mask(uint8_t port, uint8_t pinmask, uint8_t *value);
void pca_9xxx_isr_attach(uint8_t pin, gpio_isr_t gpio_isr, gpio_int_type_t type, void *args);
void pca_9xxx_isr_detach(uint8_t pin);
uint64_t pca_9xxx_pin_get_all();

#endif	/* PCA9698 */

#endif

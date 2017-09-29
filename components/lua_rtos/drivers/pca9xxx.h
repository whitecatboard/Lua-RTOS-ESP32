/*
 * Lua RTOS, PCA9698 driver
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

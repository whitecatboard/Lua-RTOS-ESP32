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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_attr.h"

#include <driver/gpio.h>

#include <stdint.h>

#include <sys/driver.h>

// Number of banks
#define PCA9698_BANKS 5

// Number of pins
#define PCA9698_PINS 40

// Convert a GPIO number to it's bank number
#define PCA9698_GPIO_BANK_NUM(gpio) (gpio >> 3)

// Convert a GPIO number to it's position into it's bank
#define PCA9698_GPIO_BANK_POS(gpio) (gpio % 7)

typedef struct {
	uint8_t direction[PCA9698_BANKS];
	uint8_t latch[PCA9698_BANKS];
	gpio_isr_t isr_func[PCA9698_PINS];
	uint8_t isr_type[PCA9698_PINS];
	void *isr_args[PCA9698_PINS];
	xQueueHandle queue;
	TaskHandle_t task;
	SemaphoreHandle_t mtx;
} pca_9698_t;

// PCA9698 errors
#define PCA9698_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(PCA9698_DRIVER_ID) |  0)

extern const int pca9698_erros;
extern const int pca9698_error_map;

driver_error_t *pca9698_setup();
void pca_9698_pin_output(uint8_t pin);
void pca_9698_pin_input(uint8_t pin);
void pca_9698_pin_set(uint8_t pin);
void pca_9698_pin_clr(uint8_t pin);
void pca_9698_pin_inv(uint8_t pin);
uint8_t pca_9698_pin_get(uint8_t pin);
void pca_9698_pin_input_mask(uint8_t port, uint8_t pinmask);
void pca_9698_pin_output_mask(uint8_t port, uint8_t pinmask);
void pca_9698_pin_set_mask(uint8_t port, uint8_t pinmask);
void pca_9698_pin_clr_mask(uint8_t port, uint8_t pinmask);
void pca_9698_pin_inv_mask(uint8_t port, uint8_t pinmask);
void pca_9698_pin_get_mask(uint8_t port, uint8_t pinmask, uint8_t *value);
void pca_9698_isr_attach(uint8_t pin, gpio_isr_t gpio_isr, gpio_int_type_t type, void *args);
void pca_9698_isr_detach(uint8_t pin);

#endif	/* PCA9698 */

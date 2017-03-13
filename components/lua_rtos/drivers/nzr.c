/*
 * Lua RTOS, NZR driver
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

#include "freertos/FreeRTOS.h"
#include "nzr.h"

#include <string.h>

#include <sys/driver.h>
#include <sys/delay.h>
#include <sys/list.h>

#include <drivers/gpio.h>

// Driver errors
DRIVER_REGISTER_ERROR(NZR, nzr, NotEnoughtMemory, "not enough memory", NZR_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_ERROR(NZR, nzr, InvalidUnit, "invalid unit", NRZ_ERR_INVALID_UNIT);

// List of units
struct list nzr_list;

/*
 * Operation functions
 */
void nzr_init() {
	// Init  list
    list_init(&nzr_list, 0);
}

driver_error_t *nzr_setup(nzr_timing_t *timing, uint8_t gpio, uint32_t *unit) {
	driver_error_t *error;
	nzr_instance_t *instance;
    driver_unit_lock_error_t *lock_error = NULL;

	// Allocate space for instance
	instance = (nzr_instance_t *)calloc(1, sizeof(nzr_instance_t));
	if (!instance) {
		return driver_operation_error(NZR_DRIVER, NZR_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Copy values to instance
	memcpy(&instance->timings, timing, sizeof(nzr_timing_t));
	instance->gpio = gpio;

	// Add instance
	if (list_add(&nzr_list, instance, (int *)unit)) {
		free(instance);

		return driver_setup_error(NZR_DRIVER, NZR_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

    // Lock the GPIO
    if ((lock_error = driver_lock(NZR_DRIVER, *unit, GPIO_DRIVER, gpio))) {
    	list_remove(&nzr_list, *unit, 1);
    	// Revoked lock on pin
    	return driver_lock_error(NZR_DRIVER, lock_error);
    }

	// Configure GPIO as output
	if ((error = gpio_pin_output(gpio))) {
		return error;
	}

	gpio_ll_pin_clr(gpio);

	return NULL;
}

driver_error_t *nzr_send(uint32_t unit, uint8_t *data, uint32_t bits) {
	nzr_instance_t *instance;
	uint8_t mask;
	uint32_t pulseH;
	uint32_t pulseL;
	uint32_t c, s, bit;

	// Get instance
    if (list_get(&nzr_list, (int)unit, (void **)&instance)) {
		return driver_operation_error(NZR_DRIVER, NRZ_ERR_INVALID_UNIT, NULL);
    }

	portDISABLE_INTERRUPTS();

	mask = 0x80;
	c = xthal_get_ccount();
	for(bit = 0;bit < bits; bit++) {
		s = c;
		pulseH = ((*data) & mask)?instance->timings.t1h:instance->timings.t0h;
		pulseL = ((*data) & mask)?instance->timings.t1l:instance->timings.t0l;

		gpio_ll_pin_set(instance->gpio);
		while (((c = xthal_get_ccount()) - s) < pulseH);

		s = c;
		gpio_ll_pin_clr(instance->gpio);
		while (((c = xthal_get_ccount()) - s) < pulseL);

		mask = mask >> 1;
		if (mask == 0) {
			mask = 0x80;
			data++;
		}
	}

	s = c;
	while (((c = xthal_get_ccount()) - s) < instance->timings.res);

	portENABLE_INTERRUPTS();

	return NULL;
}

DRIVER_REGISTER(NZR,nzr,NULL,nzr_init,NULL);

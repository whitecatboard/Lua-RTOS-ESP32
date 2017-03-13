/*
 * Lua RTOS, NEOPIXEL WS2812B driver
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

#include "neopixel.h"

#include <sys/list.h>
#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/gpio.h>
#include <drivers/nzr.h>

#define NEO_CYCLES(n) ((double)n / (double)((double)1000000000L / (double)CPU_HZ))

static nzr_timing_t chipset[1] = {
	{NEO_CYCLES(350), NEO_CYCLES(900), NEO_CYCLES(900), NEO_CYCLES(350), NEO_CYCLES(50000)}, // WS2812B
};

// Driver errors
DRIVER_REGISTER_ERROR(NEOPIXEL, neopixel, NotEnoughtMemory, "not enough memory", NEOPIXEL_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_ERROR(NEOPIXEL, neopixel, InvalidUnit, "invalid unit", NEOPIXEL_ERR_INVALID_UNIT);
DRIVER_REGISTER_ERROR(NEOPIXEL, neopixel, InvalidPixel, "invalid pixel", NEOPIXEL_ERR_INVALID_PIXEL);
DRIVER_REGISTER_ERROR(NEOPIXEL, neopixel, InvalidController, "invalid controller", NEOPIXEL_ERR_INVALID_CONTROLLER);
DRIVER_REGISTER_ERROR(NEOPIXEL, neopixel, InvalidRGBComponent, "invalid RGB component", NEOPIXEL_ERR_INVALID_RGB_COMPONENT);

// List of units
struct list neopixel_list;

/*
 * Operation functions
 */
void neopixel_init() {
	// Init  list
    list_init(&neopixel_list, 0);
}

driver_error_t *neopixel_rgb(uint32_t unit, uint32_t pixel, uint8_t r, uint8_t g, uint8_t b) {
	neopixel_instance_t *instance;

	// Get instance
    if (list_get(&neopixel_list, (int)unit, (void **)&instance)) {
		return driver_operation_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_INVALID_UNIT, NULL);
    }

    if (pixel > instance->npixels) {
		return driver_operation_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_INVALID_PIXEL, NULL);
    }

    instance->pixels[pixel].r = r;
    instance->pixels[pixel].g = g;
    instance->pixels[pixel].b = b;

    return NULL;
}

driver_error_t *neopixel_setup(neopixel_controller_t controller, uint8_t gpio, uint32_t pixels, uint32_t *unit) {
	driver_error_t *error;
	uint32_t nzr_unit;

	if (controller > 1) {
		return driver_operation_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_INVALID_CONTROLLER, NULL);
	}

	// Setup NZR
	if ((error = nzr_setup(&chipset[controller], gpio, &nzr_unit))) {
		return error;
	}

	// Create an instance
	neopixel_instance_t *instance = (neopixel_instance_t *)calloc(1,sizeof(neopixel_instance_t));
	if (!instance) {
		return driver_operation_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Populate instance
	instance->npixels = pixels;
	instance->nzr_unit = nzr_unit;
	instance->pixels = (neopixel_pixel_t *)calloc(1,sizeof(neopixel_pixel_t) * pixels);
	if (!instance->pixels) {
		free(instance);
		return driver_operation_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Add instance
	if (list_add(&neopixel_list, instance, (int *)unit)) {
		free(instance->pixels);
		free(instance);

		return driver_setup_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	return NULL;
}

driver_error_t *neopixel_update(uint32_t unit) {
	neopixel_instance_t *instance;
	driver_error_t *error;

	// Get instance
    if (list_get(&neopixel_list, (int)unit, (void **)&instance)) {
		return driver_operation_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_INVALID_UNIT, NULL);
    }

    // Send buffer
    if ((error = nzr_send(instance->nzr_unit, (uint8_t *)instance->pixels, 24 * instance->npixels))) {
		return error;
	}

	return NULL;
}

DRIVER_REGISTER(NEOPIXEL,neopixel,NULL,neopixel_init,NULL);

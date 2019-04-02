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
 * Lua RTOS, NEOPIXEL WS2812B driver
 *
 */

#include "freertos/FreeRTOS.h"

#include "neopixel.h"

#include <sys/list.h>
#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/gpio.h>
#include <drivers/nzr.h>

#define NEO_CYCLES(n) ((double)n / (double)((double)1000000000L / (double)CPU_HZ))

#define NEO_TIMING(t0h, t0l, t1h, t1l, res) \
{\
	{t0h, t0l, t1h, t1l},\
	{NEO_CYCLES(t0h), NEO_CYCLES(t0l), NEO_CYCLES(t1h), NEO_CYCLES(t1l)},\
	res\
}

static nzr_timing_t chipset[1] = {
	NEO_TIMING(350, 900, 900, 350, 50000), // WS2812B
};

// Register driver and messages
void neopixel_init();

DRIVER_REGISTER_BEGIN(NEOPIXEL,neopixel,0,neopixel_init,NULL);
	DRIVER_REGISTER_ERROR(NEOPIXEL, neopixel, NotEnoughtMemory, "not enough memory", NEOPIXEL_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(NEOPIXEL, neopixel, InvalidUnit, "invalid unit", NEOPIXEL_ERR_INVALID_UNIT);
	DRIVER_REGISTER_ERROR(NEOPIXEL, neopixel, InvalidPixel, "invalid pixel", NEOPIXEL_ERR_INVALID_PIXEL);
	DRIVER_REGISTER_ERROR(NEOPIXEL, neopixel, InvalidController, "invalid controller", NEOPIXEL_ERR_INVALID_CONTROLLER);
	DRIVER_REGISTER_ERROR(NEOPIXEL, neopixel, InvalidRGBComponent, "invalid RGB component", NEOPIXEL_ERR_INVALID_RGB_COMPONENT);
DRIVER_REGISTER_END(NEOPIXEL,neopixel,0,neopixel_init,NULL);

// List of units
struct list neopixel_list;

/*
 * Operation functions
 */
void neopixel_init() {
	// Init  list
	lstinit(&neopixel_list, 0, LIST_DEFAULT);
}

driver_error_t *neopixel_rgb(uint32_t unit, uint32_t pixel, uint8_t r, uint8_t g, uint8_t b) {
	neopixel_instance_t *instance;

	// Get instance
    if (lstget(&neopixel_list, (int)unit, (void **)&instance)) {
		return driver_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_INVALID_UNIT, NULL);
    }

    if (pixel > instance->npixels) {
		return driver_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_INVALID_PIXEL, NULL);
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
		return driver_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_INVALID_CONTROLLER, NULL);
	}

	// Setup NZR
	if ((error = nzr_setup(&chipset[controller], gpio, &nzr_unit))) {
		return error;
	}

	// Create an instance
	neopixel_instance_t *instance = (neopixel_instance_t *)calloc(1,sizeof(neopixel_instance_t));
	if (!instance) {
		return driver_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Populate instance
	instance->npixels = pixels;
	instance->nzr_unit = nzr_unit;
	instance->pixels = (neopixel_pixel_t *)calloc(1,sizeof(neopixel_pixel_t) * pixels);
	if (!instance->pixels) {
		free(instance);
		return driver_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Add instance
	if (lstadd(&neopixel_list, instance, (int *)unit)) {
		free(instance->pixels);
		free(instance);

		return driver_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	return NULL;
}

driver_error_t *neopixel_update(uint32_t unit) {
	neopixel_instance_t *instance;
	driver_error_t *error;

	// Get instance
    if (lstget(&neopixel_list, (int)unit, (void **)&instance)) {
		return driver_error(NEOPIXEL_DRIVER, NEOPIXEL_ERR_INVALID_UNIT, NULL);
    }

    // Send buffer
    if ((error = nzr_send(instance->nzr_unit, (uint8_t *)instance->pixels, 24 * instance->npixels))) {
		return error;
	}

	return NULL;
}

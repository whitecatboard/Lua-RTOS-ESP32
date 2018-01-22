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
 * Lua RTOS, segment display library
 *
 */

#include "sdisplay.h"

#include <stdarg.h>
#include <stdint.h>

#include <drivers/tm1637.h>
#include <drivers/ht16k33.h>

// Supported segment display devices
static const sdisplay_t displaydevs[] = {
	{CHIPSET_TM1637, SDisplayTwoWire, tm1637_setup, tm1637_clear, tm1637_write, tm1637_brightness},
	{CHIPSET_HT16K3, SDisplayI2C, ht16k33_setup, ht16k33_clear, ht16k33_write, ht16k33_brightness},
	{NULL}
};

// Register drivers and errors
DRIVER_REGISTER_BEGIN(SDISPLAY,sdisplay,NULL,NULL,NULL);
	DRIVER_REGISTER_ERROR(SDISPLAY, sdisplay, InvalidChipset, "invalid chipset", SDISPLAY_ERR_INVALID_CHIPSET);
	DRIVER_REGISTER_ERROR(SDISPLAY, sdisplay, TimeOut, "timeout", SDISPLAY_ERR_TIMEOUT);
	DRIVER_REGISTER_ERROR(SDISPLAY, sdisplay, InvalidBrightness, "invalid brightness", SDISPLAY_ERR_INVALID_BRIGHTNESS);
	DRIVER_REGISTER_ERROR(SDISPLAY, sdisplay, NotEnoughMemory, "not enough memory", SDISPLAY_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(SDISPLAY, sdisplay, InvalidDigits, "invalid digits", SDISPLAY_ERR_INVALID_DIGITS);
DRIVER_REGISTER_END(SDISPLAY,sdisplay,NULL,NULL,NULL);

static const sdisplay_t *sdisplay_get(uint8_t chipset) {
	const sdisplay_t *cdisplay;
	int i = 0;

	cdisplay = &displaydevs[0];
	while (cdisplay->chipset) {
		if (cdisplay->chipset == chipset) {
			return cdisplay;
		}

		cdisplay++;
		i++;
	}

	return NULL;
}

sdisplay_type_t sdisplay_type(uint8_t chipset) {
	const sdisplay_t *display = sdisplay_get(chipset);

	if (display) {
		return display->type;
	}

	return SDisplayNoType;
}

driver_error_t *sdisplay_setup(uint8_t chipset, sdisplay_device_t *device, uint8_t digits, ...) {
	const sdisplay_t *display = sdisplay_get(chipset);
	driver_error_t *error;

	// Sanity checks
	if (!display) {
		return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_INVALID_CHIPSET, NULL);
	}

	if (digits == 0) {
		return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_INVALID_DIGITS, "must be at least 1");
	}

	if (digits > 30) {
		return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_INVALID_DIGITS, "must be less than 30");
	}

	// Store display type and id
	device->display = display;
	device->id = 0;
	device->digits = digits;

	// Stote display config
	va_list args;
	va_start(args, digits);

	if (display->type == SDisplayTwoWire) {
		device->config.wire.clk = va_arg(args, int);
		device->config.wire.dio = va_arg(args, int);
	} else if (display->type == SDisplayI2C) {
		device->config.i2c.address = va_arg(args, int);
	}

	va_end(args);

	// Setup display type
	error = display->setup(device);

	return error;
}

driver_error_t *sdisplay_clear(sdisplay_device_t *device) {
	return device->display->clear(device);
}

driver_error_t *sdisplay_write(sdisplay_device_t *device, const char *data) {
	return device->display->write(device, data);
}

driver_error_t *sdisplay_brightness(sdisplay_device_t *device, uint8_t brightness) {
	return device->display->brightness(device, brightness);
}

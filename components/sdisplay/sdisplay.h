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

#ifndef SDISPLAY_H_
#define SDISPLAY_H_

#include <sys/driver.h>

typedef enum {
	SDisplayNoType = 0,
	SDisplayTwoWire = 1,
	SDisplayI2C = 2
} sdisplay_type_t;

struct sdisplay;

typedef struct {
	uint8_t chipset; // Chipset
	sdisplay_type_t type;
	driver_error_t *(*setup)(struct sdisplay *);
	driver_error_t *(*clear)(struct sdisplay *);
	driver_error_t *(*write)(struct sdisplay *, const char *);
	driver_error_t *(*brightness)(struct sdisplay *, uint8_t);
} sdisplay_t;

typedef struct sdisplay{
	const sdisplay_t *display;
	int id;
	uint8_t brightness;
	uint8_t digits;

	union {
		struct {
			int clk;
			int dio;
		} wire;

		struct {
			int device;
			int address;
		} i2c;
	} config;
} sdisplay_device_t;

#define CHIPSET_TM1637 1
#define CHIPSET_HT16K3 2

sdisplay_type_t sdisplay_type(uint8_t chipset);
driver_error_t *sdisplay_setup(uint8_t chipset, sdisplay_device_t *device, uint8_t digits, ...) ;
driver_error_t *sdisplay_clear(sdisplay_device_t *device);
driver_error_t *sdisplay_write(sdisplay_device_t *device, const char *data);
driver_error_t *sdisplay_brightness(sdisplay_device_t *device, uint8_t brightness);

//  Driver errors
#define  SDISPLAY_ERR_INVALID_CHIPSET     (DRIVER_EXCEPTION_BASE(SDISPLAY_DRIVER_ID) |  0)
#define  SDISPLAY_ERR_TIMEOUT             (DRIVER_EXCEPTION_BASE(SDISPLAY_DRIVER_ID) |  1)
#define  SDISPLAY_ERR_INVALID_BRIGHTNESS  (DRIVER_EXCEPTION_BASE(SDISPLAY_DRIVER_ID) |  2)
#define  SDISPLAY_ERR_NOT_ENOUGH_MEMORY   (DRIVER_EXCEPTION_BASE(SDISPLAY_DRIVER_ID) |  3)
#define  SDISPLAY_ERR_INVALID_DIGITS      (DRIVER_EXCEPTION_BASE(SDISPLAY_DRIVER_ID) |  4)

extern const int sdisplay_errors;
extern const int sdisplay_error_map;

#endif /* SDISPLAY_H_ */

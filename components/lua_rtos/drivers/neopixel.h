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

#ifndef NEOPIXEL_H_
#define NEOPIXEL_H_

#include <sys/driver.h>

typedef enum {
	NeopixelWS2812B,
} neopixel_controller_t;

typedef struct {
	uint8_t g;
	uint8_t r;
	uint8_t b;
} neopixel_pixel_t;

typedef struct {
	uint32_t nzr_unit;
	neopixel_pixel_t *pixels;
	uint32_t npixels;
} neopixel_instance_t;

// NEOPIXEL errors
#define NEOPIXEL_ERR_NOT_ENOUGH_MEMORY           (DRIVER_EXCEPTION_BASE(NEOPIXEL_DRIVER_ID) |  0)
#define NEOPIXEL_ERR_INVALID_UNIT                (DRIVER_EXCEPTION_BASE(NEOPIXEL_DRIVER_ID) |  1)
#define NEOPIXEL_ERR_INVALID_PIXEL               (DRIVER_EXCEPTION_BASE(NEOPIXEL_DRIVER_ID) |  2)
#define NEOPIXEL_ERR_INVALID_CONTROLLER          (DRIVER_EXCEPTION_BASE(NEOPIXEL_DRIVER_ID) |  4)
#define NEOPIXEL_ERR_INVALID_RGB_COMPONENT       (DRIVER_EXCEPTION_BASE(NEOPIXEL_DRIVER_ID) |  5)

extern const int neopixel_errors;
extern const int neopixel_error_map;

driver_error_t *neopixel_rgb(uint32_t unit, uint32_t pixel, uint8_t r, uint8_t g, uint8_t b);
driver_error_t *neopixel_setup(neopixel_controller_t controller, uint8_t gpio, uint32_t pixels, uint32_t *unit);
driver_error_t *neopixel_update(uint32_t unit);

#endif /* NEOPIXEL_H_ */

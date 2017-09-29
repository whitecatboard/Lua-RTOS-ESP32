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

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
 * Lua RTOS, line primitive
 *
 */

/*
 * This functions are taken from:
 *
 * Boris Lovošević, tft driver for Lua RTOS:
 *
 * https://github.com/loboris/Lua-RTOS-ESP32-lobo/tree/master/components/lua_rtos/Lua/modules/screen
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

#include <gdisplay/gdisplay.h>

/*
 * Helper functions
 */

/*
 * Operation functions
 */
driver_error_t *gdisplay_hline(int x0, int y0, int w, uint32_t color) {
	int x;

	if (w < 0) {
		w = -1 * w;
		x0 = x0 - w;
	}

	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	gdisplay_begin();

	for (x = x0; x < x0 + w; x++) {
		gdisplay_set_pixel(x,y0,color);
	}

	gdisplay_end();

	return NULL;
}

driver_error_t *gdisplay_vline(int x0, int y0, int h, uint32_t color) {
	int y;

	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	if (h < 0) {
		h = -1 * h;
		y0 = y0 - h;
	}

	gdisplay_begin();

	for (y = y0; y < y0 + h; y++) {
		gdisplay_set_pixel(x0,y,color);
	}

	gdisplay_end();

	return NULL;
}

driver_error_t *gdisplay_line(int x0, int y0, int x1, int y1, uint32_t color) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	if (x0 == x1) {
		if (y0 <= y1)
			gdisplay_vline(x0, y0, y1 - y0 + 1, color);
		else
			gdisplay_vline(x0, y1, y0 - y1 + 1, color);

		return NULL;
	}

	if (y0 == y1) {
		if (x0 <= x1)
			gdisplay_hline(x0, y0, x1 - x0 + 1, color);
		else
			gdisplay_hline(x1, y0, x0 - x1 + 1, color);

		return NULL;
	}

	int steep = 0;
	if (abs(y1 - y0) > abs(x1 - x0))
		steep = 1;
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}
	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx = x1 - x0, dy = abs(y1 - y0);
	int16_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;

	if (y0 < y1)
		ystep = 1;

	gdisplay_begin();

	// Split into steep and not steep for FastH/V separation
	if (steep) {
		for (; x0 <= x1; x0++) {
			dlen++;
			err -= dy;
			if (err < 0) {
				err += dx;
				if (dlen == 1)
					gdisplay_set_pixel(y0, xs, color);
				else
					gdisplay_vline(y0, xs, dlen, color);
				dlen = 0;
				y0 += ystep;
				xs = x0 + 1;
			}
		}
		if (dlen)
			gdisplay_vline(y0, xs, dlen, color);
	} else {
		for (; x0 <= x1; x0++) {
			dlen++;
			err -= dy;
			if (err < 0) {
				err += dx;
				if (dlen == 1)
					gdisplay_set_pixel(xs, y0, color);
				else
					gdisplay_hline(xs, y0, dlen, color);
				dlen = 0;
				y0 += ystep;
				xs = x0 + 1;
			}
		}
		if (dlen)
			gdisplay_hline(xs, y0, dlen, color);
	}
	gdisplay_end();

	return NULL;
}

driver_error_t *gdisplay_line_by_angle(int x, int y, int length, int angle, int offset, int start, uint32_t color) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	if (start >= length) start = length - 1;

	if (start == 0) {
		gdisplay_line(
			x, y,
			(double)x + (double)length * cos((double)(angle + offset) * DEG_TO_RAD),
			(double)y + (double)length * sin((double)(angle + offset) * DEG_TO_RAD), color);
	} else {
		gdisplay_line(
			(double)x + (double)start * cos((double)(angle + offset) * DEG_TO_RAD),
			(double)y + (double)start * sin((double)(angle + offset) * DEG_TO_RAD),
			(double)x + (double)(start + length) * cos((double)(angle + offset) * DEG_TO_RAD),
			(double)y + (double)(start + length) * sin((double)(angle + offset) * DEG_TO_RAD), color);
	}

	return NULL;
}

#endif

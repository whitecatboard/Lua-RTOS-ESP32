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
 * Lua RTOS, circle primitive
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
#include "primitives.h"

/*
 * Helper functions
 */
void _gdisplay_circle_helper(int x0, int y0, int r, uint8_t cornername, uint32_t color) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		if (cornername & 0x4) {
			gdisplay_set_pixel(x0 + x, y0 + y, color);
			gdisplay_set_pixel(x0 + y, y0 + x, color);
		}
		if (cornername & 0x2) {
			gdisplay_set_pixel(x0 + x, y0 - y, color);
			gdisplay_set_pixel(x0 + y, y0 - x, color);
		}
		if (cornername & 0x8) {
			gdisplay_set_pixel(x0 - y, y0 + x, color);
			gdisplay_set_pixel(x0 - x, y0 + y, color);
		}
		if (cornername & 0x1) {
			gdisplay_set_pixel(x0 - y, y0 - x, color);
			gdisplay_set_pixel(x0 - x, y0 - y, color);
		}
	}
}

void _gdisplay_circle_fill_helper(int x0, int y0, int r, uint8_t cornername, int16_t delta, uint32_t color) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;
	int16_t ylm = x0 - r;

	while (x < y) {
		if (f >= 0) {
			if (cornername & 0x1) gdisplay_vline(x0 + y, y0 - x, 2 * x + 1 + delta, color);
			if (cornername & 0x2) gdisplay_vline(x0 - y, y0 - x, 2 * x + 1 + delta, color);
			ylm = x0 - y;
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		if ((x0 - x) > ylm) {
			if (cornername & 0x1) gdisplay_vline(x0 + x, y0 - y, 2 * y + 1 + delta, color);
			if (cornername & 0x2) gdisplay_vline(x0 - x, y0 - y, 2 * y + 1 + delta, color);
		}
	}
}

/*
 * Operation functions
 */
driver_error_t *gdisplay_circle(int x, int y, int radius, uint32_t color) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	if (radius < 0) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_INVALID_RADIUS, NULL);
	}

	gdisplay_begin();
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x1 = 0;
	int y1 = radius;

	gdisplay_set_pixel(x, y + radius, color);
	gdisplay_set_pixel(x, y - radius, color);
	gdisplay_set_pixel(x + radius, y, color);
	gdisplay_set_pixel(x - radius, y, color);

	while (x1 < y1) {
		if (f >= 0) {
			y1--;
			ddF_y += 2;
			f += ddF_y;
		}
		x1++;
		ddF_x += 2;
		f += ddF_x;
		gdisplay_set_pixel(x + x1, y + y1, color);
		gdisplay_set_pixel(x - x1, y + y1, color);
		gdisplay_set_pixel(x + x1, y - y1, color);
		gdisplay_set_pixel(x - x1, y - y1, color);
		gdisplay_set_pixel(x + y1, y + x1, color);
		gdisplay_set_pixel(x - y1, y + x1, color);
		gdisplay_set_pixel(x + y1, y - x1, color);
		gdisplay_set_pixel(x - y1, y - x1, color);
	}	gdisplay_end();

	return NULL;
}

driver_error_t *gdisplay_circle_fill(int x, int y, int radius, uint32_t color, uint32_t fill) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	if (radius < 0) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_INVALID_RADIUS, NULL);
	}

	gdisplay_begin();

	gdisplay_vline(x, y-radius, 2*radius+1, fill);
	_gdisplay_circle_fill_helper(x, y, radius, 3, 0, fill);

	gdisplay_circle(x, y, radius, color);

	gdisplay_end();

	return NULL;
}

#endif

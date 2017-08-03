/*
 * Lua RTOS, circle primitive
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

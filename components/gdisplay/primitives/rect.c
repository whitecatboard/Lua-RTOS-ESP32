/*
 * Lua RTOS, rect primitive
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

driver_error_t *gdisplay_rect(int x0, int y0, int w, int h, uint32_t color) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	if (w < 0) {
		w = -1 * w;
		x0 = x0 - w;
	}

	if (h < 0) {
		h = -1 * h;
		y0 = y0 - h;
	}

	// Draw border
	gdisplay_begin();

	gdisplay_hline(x0, y0, w, color);
	gdisplay_hline(x0, y0 + h - 1, w, color);
	gdisplay_vline(x0, y0, h, color);
	gdisplay_vline(x0 + w - 1, y0, h, color);

	gdisplay_end();

	return NULL;
}

driver_error_t *gdisplay_rect_fill(int x0, int y0, int w, int h, uint32_t color, uint32_t fill) {
	int x, y;

	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	if (w < 0) {
		w = -1 * w;
		x0 = x0 - w;
	}

	if (h < 0) {
		h = -1 * h;
		y0 = y0 - h;
	}

	// Draw border
	gdisplay_begin();

	gdisplay_hline(x0, y0, w, color);
	gdisplay_hline(x0, y0 + h - 1, w, color);
	gdisplay_vline(x0, y0, h, color);
	gdisplay_vline(x0 + w - 1, y0, h, color);

	// Fill
	for (y = y0 + 1; y < y0 + h - 1; y++) {
		for (x = x0 + 1; x < x0 + w - 1; x++) {
			gdisplay_set_pixel(x,y,fill);
		}
	}

	gdisplay_end();

	return NULL;
}

driver_error_t *gdisplay_round_rect(int x, int y, int w, int h, int r, uint32_t color) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	gdisplay_begin();

	// smarter version
	gdisplay_hline(x + r, y, w - 2 * r, color);			// Top
	gdisplay_hline(x + r, y + h - 1, w - 2 * r, color);	// Bottom
	gdisplay_vline(x, y + r, h - 2 * r, color);			// Left
	gdisplay_vline(x + w - 1, y + r, h - 2 * r, color);	// Right

	// draw four corners
	_gdisplay_circle_helper(x + r, y + r, r, 1, color);
	_gdisplay_circle_helper(x + w - r - 1, y + r, r, 2, color);
	_gdisplay_circle_helper(x + w - r - 1, y + h - r - 1, r, 4, color);
	_gdisplay_circle_helper(x + r, y + h - r - 1, r, 8, color);

	gdisplay_end();

	return NULL;
}

driver_error_t *gdisplay_round_rect_fill(int x, int y, int w, int h, int r, uint32_t color, uint32_t fill) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	gdisplay_begin();

	// smarter version
	gdisplay_rect_fill(x + r, y, w  - 2 * r, h, fill, fill);

	// draw four corners
	_gdisplay_circle_fill_helper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, fill);
	_gdisplay_circle_fill_helper(x + r, y + r, r, 2, h - 2 * r - 1, fill);

	// border
	gdisplay_round_rect(x, y, w, h, r, color);

	gdisplay_end();

	return NULL;
}

#endif

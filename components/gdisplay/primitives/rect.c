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
 * Lua RTOS, rect primitive
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

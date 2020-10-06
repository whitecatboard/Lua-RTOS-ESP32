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
 * Lua RTOS, ellipse primitive
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
static void gdisplay_ellipse_section_fill(int x, int y, int x0, int y0, uint32_t color, uint8_t option)
{
    // upper right
    if ( option & TFT_ELLIPSE_UPPER_RIGHT ) gdisplay_vline(x0+x, y0-y, y+1, color);
    // upper left
    if ( option & TFT_ELLIPSE_UPPER_LEFT ) gdisplay_vline(x0-x, y0-y, y+1, color);
    // lower right
    if ( option & TFT_ELLIPSE_LOWER_RIGHT ) gdisplay_vline(x0+x, y0, y+1, color);
    // lower left
    if ( option & TFT_ELLIPSE_LOWER_LEFT ) gdisplay_vline(x0-x, y0, y+1, color);
}

static void gdisplay_ellipse_section(int x, int y, int x0, int y0, uint32_t color, uint8_t option) {
    // upper right
    if ( option & TFT_ELLIPSE_UPPER_RIGHT ) gdisplay_set_pixel(x0 + x, y0 - y, color);
    // upper left
    if ( option & TFT_ELLIPSE_UPPER_LEFT ) gdisplay_set_pixel(x0 - x, y0 - y, color);
    // lower right
    if ( option & TFT_ELLIPSE_LOWER_RIGHT ) gdisplay_set_pixel(x0 + x, y0 + y, color);
    // lower left
    if ( option & TFT_ELLIPSE_LOWER_LEFT ) gdisplay_set_pixel(x0 - x, y0 + y, color);
}

/*
 * Operation functions
 */
driver_error_t *gdisplay_ellipse(int x0, int y0, int rx, int ry, uint32_t color, uint8_t option) {
	uint16_t x, y;
	int32_t xchg, ychg;
	int32_t err;
	int32_t rxrx2;
	int32_t ryry2;
	int32_t stopx, stopy;

	rxrx2 = rx;
	rxrx2 *= rx;
	rxrx2 *= 2;

	ryry2 = ry;
	ryry2 *= ry;
	ryry2 *= 2;

	x = rx;
	y = 0;

	xchg = 1;
	xchg -= rx;
	xchg -= rx;
	xchg *= ry;
	xchg *= ry;

	ychg = rx;
	ychg *= rx;

	err = 0;

	stopx = ryry2;
	stopx *= rx;
	stopy = 0;

	gdisplay_begin();

	while (stopx >= stopy) {
		gdisplay_ellipse_section(x, y, x0, y0, color, option);
		y++;
		stopy += rxrx2;
		err += ychg;
		ychg += rxrx2;
		if (2 * err + xchg > 0) {
			x--;
			stopx -= ryry2;
			err += xchg;
			xchg += ryry2;
		}
	}

	x = 0;
	y = ry;

	xchg = ry;
	xchg *= ry;

	ychg = 1;
	ychg -= ry;
	ychg -= ry;
	ychg *= rx;
	ychg *= rx;

	err = 0;

	stopx = 0;

	stopy = rxrx2;
	stopy *= ry;

	while (stopx <= stopy) {
		gdisplay_ellipse_section(x, y, x0, y0, color, option);
		x++;
		stopx += ryry2;
		err += xchg;
		xchg += ryry2;
		if (2 * err + ychg > 0) {
			y--;
			stopy -= rxrx2;
			err += ychg;
			ychg += rxrx2;
		}
	}

	gdisplay_end();

	return NULL;
}

driver_error_t *gdisplay_ellipse_fill(int x0, int y0, int rx, int ry, uint32_t color, uint32_t fill, uint8_t option) {
	uint16_t x, y;
	int32_t xchg, ychg;
	int32_t err;
	int32_t rxrx2;
	int32_t ryry2;
	int32_t stopx, stopy;

	rxrx2 = rx;
	rxrx2 *= rx;
	rxrx2 *= 2;

	ryry2 = ry;
	ryry2 *= ry;
	ryry2 *= 2;

	x = rx;
	y = 0;

	xchg = 1;
	xchg -= rx;
	xchg -= rx;
	xchg *= ry;
	xchg *= ry;

	ychg = rx;
	ychg *= rx;

	err = 0;

	stopx = ryry2;
	stopx *= rx;
	stopy = 0;

	gdisplay_begin();

	while (stopx >= stopy) {
		gdisplay_ellipse_section_fill(x, y, x0, y0, fill, option);
		y++;
		stopy += rxrx2;
		err += ychg;
		ychg += rxrx2;
		if (2 * err + xchg > 0) {
			x--;
			stopx -= ryry2;
			err += xchg;
			xchg += ryry2;
		}
	}

	x = 0;
	y = ry;

	xchg = ry;
	xchg *= ry;

	ychg = 1;
	ychg -= ry;
	ychg -= ry;
	ychg *= rx;
	ychg *= rx;

	err = 0;

	stopx = 0;

	stopy = rxrx2;
	stopy *= ry;

	while (stopx <= stopy) {
		gdisplay_ellipse_section_fill(x, y, x0, y0, fill, option);
		x++;
		stopx += ryry2;
		err += xchg;
		xchg += ryry2;
		if (2 * err + ychg > 0) {
			y--;
			stopy -= rxrx2;
			err += ychg;
			ychg += rxrx2;
		}
	}

	gdisplay_ellipse(x0, y0, rx, ry, color, option);

	gdisplay_end();

	return NULL;
}

#endif

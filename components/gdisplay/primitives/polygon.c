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
 * Lua RTOS, polygon primitive
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
void gdisplay_polygon_helper(int cx, int cy, int sides, int diameter, uint32_t color, uint8_t fill, int deg) {
	sides = (sides > 2 ? sides : 3);    // This ensures the minimum side number is 3.
	int Xpoints[sides], Ypoints[sides];	// Set the arrays based on the number of sides entered
	int rads = 360 / sides;				// This equally spaces the points.

	for (int idx = 0; idx < sides; idx++) {
		Xpoints[idx] = cx
				+ sin((float) (idx * rads + deg) * deg_to_rad) * diameter;
		Ypoints[idx] = cy
				+ cos((float) (idx * rads + deg) * deg_to_rad) * diameter;
	}

	for (int idx = 0; idx < sides; idx++)	// draws the polygon on the screen.
			{
		if ((idx + 1) < sides)
			gdisplay_line(Xpoints[idx], Ypoints[idx], Xpoints[idx + 1],
					Ypoints[idx + 1], color); // draw the lines
		else
			gdisplay_line(Xpoints[idx], Ypoints[idx], Xpoints[0], Ypoints[0],
					color); // finishes the last line to close up the polygon.
	}
	if (fill)
		for (int idx = 0; idx < sides; idx++) {
			if ((idx + 1) < sides)
				gdisplay_triangle_fill(cx, cy, Xpoints[idx], Ypoints[idx],
						Xpoints[idx + 1], Ypoints[idx + 1], color, color);
			else
				gdisplay_triangle_fill(cx, cy, Xpoints[idx], Ypoints[idx], Xpoints[0],
						Ypoints[0], color, color);
		}

}

/*
 * Operation functions
 */
driver_error_t *gdisplay_polygon(int cx, int cy, int sides, int diameter, int deg, uint32_t color) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER,
				GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	gdisplay_begin();
	gdisplay_polygon_helper(cx, cy, sides, diameter, color, 0, deg);
	gdisplay_end();

	return NULL;
}

driver_error_t *gdisplay_polygon_fill(int cx, int cy, int sides, int diameter, int deg, uint32_t color, uint32_t fill) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER,
				GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	gdisplay_begin();
	gdisplay_polygon_helper(cx, cy, sides, diameter, fill, 1, deg);
	gdisplay_polygon_helper(cx, cy, sides, diameter, color, 0, deg);
	gdisplay_end();

	return NULL;
}

#endif

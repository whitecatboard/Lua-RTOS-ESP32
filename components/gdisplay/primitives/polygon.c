/*
 * Lua RTOS, polygon primitive
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

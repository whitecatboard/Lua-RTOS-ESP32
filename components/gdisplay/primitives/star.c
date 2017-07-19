/*
 * Lua RTOS, star primitive
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

static void gdisplay_star_helper(int cx, int cy, int diameter, uint32_t color, uint8_t fill, float factor) {
	factor = constrain(factor, 1.0, 4.0);
	uint8_t sides = 5;
	uint8_t rads = 360 / sides;

	int Xpoints_O[sides], Ypoints_O[sides], Xpoints_I[sides], Ypoints_I[sides];

	for (int idx = 0; idx < sides; idx++) {
		// makes the outer points
		Xpoints_O[idx] = cx
				+ sin((float) (idx * rads + 72) * deg_to_rad) * diameter;
		Ypoints_O[idx] = cy
				+ cos((float) (idx * rads + 72) * deg_to_rad) * diameter;
		// makes the inner points
		Xpoints_I[idx] = cx
				+ sin((float) (idx * rads + 36) * deg_to_rad)
						* ((float) (diameter) / factor);
		// 36 is half of 72, and this will allow the inner and outer points to line up like a triangle.
		Ypoints_I[idx] = cy
				+ cos((float) (idx * rads + 36) * deg_to_rad)
						* ((float) (diameter) / factor);
	}

	for (int idx = 0; idx < sides; idx++) {
		if ((idx + 1) < sides) {
			if (fill) // this part below should be self explanatory. It fills in the star.
			{
				gdisplay_triangle_fill(cx, cy, Xpoints_I[idx], Ypoints_I[idx],
						Xpoints_O[idx], Ypoints_O[idx], color, color);
				gdisplay_triangle_fill(cx, cy, Xpoints_O[idx], Ypoints_O[idx],
						Xpoints_I[idx + 1], Ypoints_I[idx + 1], color, color);
			} else {
				gdisplay_line(Xpoints_O[idx], Ypoints_O[idx],
						Xpoints_I[idx + 1], Ypoints_I[idx + 1], color);
				gdisplay_line(Xpoints_I[idx], Ypoints_I[idx], Xpoints_O[idx],
						Ypoints_O[idx], color);
			}
		} else {
			if (fill) {
				gdisplay_triangle_fill(cx, cy, Xpoints_I[0], Ypoints_I[0],
						Xpoints_O[idx], Ypoints_O[idx], color, color);
				gdisplay_triangle_fill(cx, cy, Xpoints_O[idx], Ypoints_O[idx],
						Xpoints_I[idx], Ypoints_I[idx], color, color);
			} else {
				gdisplay_line(Xpoints_O[idx], Ypoints_O[idx], Xpoints_I[idx],
						Ypoints_I[idx], color);
				gdisplay_line(Xpoints_I[0], Ypoints_I[0], Xpoints_O[idx],
						Ypoints_O[idx], color);
			}
		}
	}
}

driver_error_t *gdisplay_star(int cx, int cy, int diameter, float factor, uint32_t color) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_operation_error(GDISPLAY_DRIVER,
				GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	gdisplay_begin();

	gdisplay_star_helper(cx, cy, diameter, color, 0, factor);

	gdisplay_end();

	return NULL;
}

driver_error_t *gdisplay_star_fill(int cx, int cy, int diameter, float factor, uint32_t color, uint32_t fill) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_operation_error(GDISPLAY_DRIVER,
				GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	gdisplay_begin();

	gdisplay_star_helper(cx, cy, diameter, fill, 1, factor);
	gdisplay_star_helper(cx, cy, diameter, color, 0, factor);

	gdisplay_end();

	return NULL;
}

#endif

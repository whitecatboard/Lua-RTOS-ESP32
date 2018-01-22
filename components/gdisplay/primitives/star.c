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
 * Lua RTOS, star primitive
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
		return driver_error(GDISPLAY_DRIVER,
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
		return driver_error(GDISPLAY_DRIVER,
				GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	gdisplay_begin();

	gdisplay_star_helper(cx, cy, diameter, fill, 1, factor);
	gdisplay_star_helper(cx, cy, diameter, color, 0, factor);

	gdisplay_end();

	return NULL;
}

#endif

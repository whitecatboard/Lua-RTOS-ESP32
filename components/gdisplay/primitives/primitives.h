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
 * Lua RTOS, display primitives
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

#ifndef GDISPLAY_PRIMITIVES_H_
#define GDISPLAY_PRIMITIVES_H_

#include <sys/driver.h>

// Constants for ellipse function
#define TFT_ELLIPSE_UPPER_RIGHT 0x01
#define TFT_ELLIPSE_UPPER_LEFT  0x02
#define TFT_ELLIPSE_LOWER_LEFT 0x04
#define TFT_ELLIPSE_LOWER_RIGHT  0x08

// Constants for Arc function
// number representing the maximum angle (e.g. if 100, then if you pass in start=0 and end=50, you get a half circle)
// this can be changed with setArcParams function at runtime
#define DEFAULT_ARC_ANGLE_MAX 360

// rotational offset in degrees defining position of value 0 (-90 will put it at the top of circle)
// this can be changed with setAngleOffset function at runtime
#define DEFAULT_ANGLE_OFFSET -90

driver_error_t *gdisplay_arc(int cx, int cy, int r, int th, float start, float end, uint32_t color);
driver_error_t *gdisplay_arc_fill(int cx, int cy, int r, int th, float start, float end, uint32_t color, uint32_t fill);

driver_error_t *gdisplay_hline(int x0, int y0, int w, uint32_t color);
driver_error_t *gdisplay_vline(int x0, int y0, int h, uint32_t color);
driver_error_t *gdisplay_line(int x0, int y0, int x1, int y1, uint32_t color);
driver_error_t *gdisplay_line_by_angle(int x, int y, int length, int angle, int offset, int start, uint32_t color);

void _gdisplay_circle_helper(int x0, int y0, int r, uint8_t cornername, uint32_t color);
void _gdisplay_circle_fill_helper(int x0, int y0, int r, uint8_t cornername, int16_t delta, uint32_t color);
driver_error_t *gdisplay_circle(int x, int y, int radius, uint32_t color);
driver_error_t *gdisplay_circle_fill(int x, int y, int radius, uint32_t color, uint32_t fill);

driver_error_t *gdisplay_ellipse(int x0, int y0, int rx, int ry, uint32_t color, uint8_t option);
driver_error_t *gdisplay_ellipse_fill(int x0, int y0, int rx, int ry, uint32_t color, uint32_t fill, uint8_t option);

driver_error_t *gdisplay_polygon(int cx, int cy, int sides, int diameter, int deg, uint32_t color);
driver_error_t *gdisplay_polygon_fill(int cx, int cy, int sides, int diameter, int deg, uint32_t color, uint32_t fill);

driver_error_t *gdisplay_rect(int x0, int y0, int w, int h, uint32_t color);
driver_error_t *gdisplay_rect_fill(int x0, int y0, int w, int h, uint32_t color, uint32_t fill);
driver_error_t *gdisplay_round_rect(int x, int y, int w, int h, int r, uint32_t color);
driver_error_t *gdisplay_round_rect_fill(int x, int y, int w, int h, int r, uint32_t color, uint32_t fill);

driver_error_t *gdisplay_star(int cx, int cy, int diameter, float factor, uint32_t color);
driver_error_t *gdisplay_star_fill(int cx, int cy, int diameter, float factor, uint32_t color, uint32_t fill);

driver_error_t *gdisplay_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
driver_error_t * gdisplay_triangle_fill(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color, uint32_t fill);

#endif

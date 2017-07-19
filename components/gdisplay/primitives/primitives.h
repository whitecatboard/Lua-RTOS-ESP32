/*
 * Lua RTOS, display primitives
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

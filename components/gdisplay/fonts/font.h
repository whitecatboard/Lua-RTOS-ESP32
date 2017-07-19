/*
 * Lua RTOS, font management functions
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

#ifndef FONT_H_
#define FONT_H_

#include <stdint.h>

#include <sys/driver.h>

typedef struct {
	uint8_t 	*font;
	uint8_t 	x_size;
	uint8_t 	y_size;
	uint8_t	    offset;
	uint16_t	numchars;
    uint8_t     bitmap;
	uint16_t    color;
} font_t;

typedef struct {
      uint8_t charCode;
      int adjYOffset;
      int width;
      int height;
      int xOffset;
      int xDelta;
      uint16_t dataPtr;
} propFont;

#define DEFAULT_FONT	0
#define DEJAVU18_FONT	1
#define DEJAVU24_FONT	2
#define UBUNTU16_FONT	3
#define COMIC24_FONT	4
#define MINYA24_FONT	5
#define TOONEY32_FONT	6
#define FONT_7SEG		7
#define USER_FONT		8
#define LCD_FONT		9

driver_error_t *gdisplay_get_font_size(int *w, int *h);
driver_error_t *gdisplay_set_font(uint8_t font, const char *file);
driver_error_t *gdisplay_get_font_height(int *height);
driver_error_t *gdisplay_print(int x, int y, char *st, int color, int fill);
driver_error_t *gdisplay_get_font_size(int *w, int *h);
driver_error_t *gdisplay_string_pos(int x, int y, const char *buf, int *posx, int *posy, int *posw, int *posh);
void gdisplay_draw7seg(int x, int y, int num, int w, int l, int color, int fill, uint8_t offset);

#endif /* FONT_H_ */

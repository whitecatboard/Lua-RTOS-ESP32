/*
 * Lua RTOS, BCD font
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
 * This source code has been taken from TFT driver for Lua RTOS, authored by
 * Boris Lovošević:
 *
 * https://github.com/loboris/Lua-RTOS-ESP32-lobo/tree/master/components/lua_rtos/Lua/modules/screen
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

#include <gdisplay/gdisplay.h>
#include "font.h"
#include "primitives.h"

/**
 * bit-encoded bar position of all digits' bcd segments
 *
 *           6
 * 		  +-----+
 * 		3 |  .	| 2
 * 		  +--5--+
 * 		1 |  .	| 0
 * 		  +--.--+
 * 		     4
 */
const uint16_t font_bcd[] = {
  0x200, // 0010 0000 0000  // -
  0x080, // 0000 1000 0000  // .
  0x06C, // 0100 0110 1100  // /, degree
  0x05f, // 0000 0101 1111, // 0
  0x005, // 0000 0000 0101, // 1
  0x076, // 0000 0111 0110, // 2
  0x075, // 0000 0111 0101, // 3
  0x02d, // 0000 0010 1101, // 4
  0x079, // 0000 0111 1001, // 5
  0x07b, // 0000 0111 1011, // 6
  0x045, // 0000 0100 0101, // 7
  0x07f, // 0000 0111 1111, // 8
  0x07d, // 0000 0111 1101  // 9
  0x900  // 1001 0000 0000  // :
};

/*
 * Helper functions
 */
static void gdisplay_barVert(int x, int y, int w, int l, int color) {
	gdisplay_begin();

	gdisplay_triangle_fill(
		x + 1, y + 2 * w, x + w, y + w + 1, x + 2 * w - 1, y + 2 * w, color, color
	);

	gdisplay_triangle_fill(
		x + 1, y + 2 * w + l + 1, x + w, y + 3 * w + l, x + 2 * w - 1, y + 2 * w + l + 1, color, color
	);

	gdisplay_rect_fill(x, y + 2 * w + 1, 2 * w + 1, l, color, color);

	gdisplay_end();
}

static void gdisplay_barHor(int x, int y, int w, int l, int color) {
	gdisplay_begin();

	gdisplay_triangle_fill(x+2*w, y+2*w-1, x+w+1, y+w, x+2*w, y+1, color, color);
	gdisplay_triangle_fill(x+2*w+l+1, y+2*w-1, x+3*w+l, y+w, x+2*w+l+1, y+1, color, color);
	gdisplay_rect_fill(x+2*w+1, y, l, 2*w+1, color, color);

	gdisplay_end();
}

/*
 * Operation functions
 */
void gdisplay_draw7seg(int x, int y, int num, int w, int l, int color, int fill, uint8_t offset) {
	if (num < 0x2D || num > 0x3A)
		return;

	gdisplay_begin();

	int16_t c = font_bcd[num - 0x2D];
	int16_t d = 2 * w + l + 1;

	if (!gdisplay_get_transparency()) {
		gdisplay_rect_fill(x, y, (2 * (2 * w + 1)) + l, (3 * (2 * w + 1)) + (2 * l), fill, fill);
	}

	if ((c & 0x001))
		gdisplay_barVert(x + d, y + d, w, l, color);
	if ((c & 0x002))
		gdisplay_barVert(x, y + d, w, l, color);
	if ((c & 0x004))
		gdisplay_barVert( x + d, y, w, l, color);
	if ((c & 0x008))
		gdisplay_barVert(x, y, w, l, color);
	if ((c & 0x010))
		gdisplay_barHor(x, y + 2 * d, w, l, color);
	if ((c & 0x020))
		gdisplay_barHor(x, y + d, w, l, color);
	if ((c & 0x040))
		gdisplay_barHor(x, y, w, l, color);

	if ((c & 0x100))
		gdisplay_rect_fill(x + (d / 2), y + d + 2 * w + 1, 2 * w + 1, l / 2, color, color);

	if ((c & 0x800))
		gdisplay_rect_fill(x + (d / 2), y + (2 * w) + 1 + (l / 2), 2 * w + 1, l / 2, color, color);

	if (c & 0x001)
		gdisplay_barVert(x + d, y + d, w, l, color);          // down right

	if (c & 0x002)
		gdisplay_barVert(x, y + d, w, l, color);              // down left

	if (c & 0x004)
		gdisplay_barVert(x + d, y, w, l, color);              // up right

	if (c & 0x008)
		gdisplay_barVert(x, y, w, l, color);                  // up left
	if (c & 0x010)
		gdisplay_barHor(x, y + 2 * d, w, l, color);           // down
	if (c & 0x020)
		gdisplay_barHor(x, y + d, w, l, color);               // middle
	if (c & 0x040)
		gdisplay_barHor(x, y, w, l, color);                   // up

	if (c & 0x080) {
		gdisplay_rect_fill(x + (d / 2), y + 2 * d, 2 * w + 1, 2 * w + 1, color, color); // low point

		if (offset)
			gdisplay_rect(x + (d / 2), y + 2 * d, 2 * w + 1, 2 * w + 1, color);
	}

	if (c & 0x100) {
		gdisplay_rect_fill(x + (d / 2), y + d + 2 * w + 1, 2 * w + 1, l / 2, color, color); // down middle point
		if (offset)
			gdisplay_rect(x + (d / 2), y + d + 2 * w + 1, 2 * w + 1, l / 2, color);
	}

	if (c & 0x800) {
		gdisplay_rect_fill(x + (d / 2), y + (2 * w) + 1 + (l / 2), 2 * w + 1, l / 2, color, color); // up middle point
		if (offset)
			gdisplay_rect(x + (d / 2), y + (2 * w) + 1 + (l / 2), 2 * w + 1, l / 2, color);
	}

	if (c & 0x200) {
		gdisplay_rect_fill(x + 2 * w + 1, y + d, l, 2 * w + 1, color, color); // middle, minus
		if (offset)
			gdisplay_rect(x + 2 * w + 1, y + d, l, 2 * w + 1, color);
	}

	gdisplay_end();
}

#endif

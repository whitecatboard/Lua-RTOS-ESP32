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
 * Lua RTOS, font management functions
 *
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

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <sys/stat.h>
#include <sys/syslog.h>

// Default fonts
extern uint8_t tft_DefaultFont[];
extern uint8_t tft_Dejavu18[];
extern uint8_t tft_Dejavu24[];
extern uint8_t tft_Ubuntu16[];
extern uint8_t tft_Comic24[];
extern uint8_t tft_minya24[];
extern uint8_t tft_tooney32[];
extern uint16_t lcd_5[];
extern uint16_t font_bcd[];

// Current font
font_t   cfont;
propFont fontChar;
static uint8_t *userfont = NULL;

static int load_file_font(const char * fontfile, int info)
{
	if (userfont != NULL) {
		free(userfont);
		userfont = NULL;
	}

    struct stat sb;

    // Open the file
    FILE *fhndl = fopen(fontfile, "r");
    if (!fhndl) {
    	syslog(LOG_ERR, "Error opening font file '%s'", fontfile);
		return 0;
    }

	// Get file size
    if (stat(fontfile, &sb) != 0) {
    	syslog(LOG_ERR, "Error getting font file size");
		fclose(fhndl);
		return 0;
    }
	int fsize = sb.st_size;
	if (fsize < 30) {
		syslog(LOG_ERR, "Error getting font file size");
		fclose(fhndl);
		return 0;
	}

	userfont = malloc(fsize+4);
	if (userfont == NULL) {
		syslog(LOG_ERR, "Font memory allocation error");
		fclose(fhndl);
		return 0;
	}

	int read = fread(userfont, 1, fsize, fhndl);

	fclose(fhndl);

	if (read != fsize) {
		syslog(LOG_ERR, "Font read error");
		free(userfont);
		userfont = NULL;
		return 0;
	}

	userfont[read] = 0;
	if (strstr((char *)(userfont+read-8), "RPH_font") == NULL) {
		syslog(LOG_ERR, "Font ID not found");
		free(userfont);
		userfont = NULL;
		return 0;
	}

	// Check size
	int size = 0;
	int numchar = 0;
	int width = userfont[0];
	int height = userfont[1];
	uint8_t first = 255;
	uint8_t last = 0;
	//int offst = 0;
	int pminwidth = 255;
	int pmaxwidth = 0;

	if (width != 0) {
		// Fixed font
		numchar = userfont[3];
		first = userfont[2];
		last = first + numchar - 1;
		size = ((width * height * numchar) / 8) + 4;
	}
	else {
		// Proportional font
		size = 4; // point at first char data
		uint8_t charCode;
		int charwidth;

		do {
		    charCode = userfont[size];
		    charwidth = userfont[size+2];

		    if (charCode != 0xFF) {
		    	numchar++;
		    	if (charwidth != 0) size += ((((charwidth * userfont[size+3])-1) / 8) + 7);
		    	else size += 6;

		    	if (info) {
	    			if (charwidth > pmaxwidth) pmaxwidth = charwidth;
	    			if (charwidth < pminwidth) pminwidth = charwidth;
	    			if (charCode < first) first = charCode;
	    			if (charCode > last) last = charCode;
	    		}
		    }
		    else size++;
		  } while ((size < (read-8)) && (charCode != 0xFF));
	}

	if (size != (read-8)) {
		syslog(LOG_ERR, "Font size error: found %d expected %d)", size, (read-8));
		free(userfont);
		userfont = NULL;
		return 0;
	}

	if (info) {
		if (width != 0) {
			syslog(LOG_INFO, "Fixed width font:\r\n  size: %d  width: %d  height: %d  characters: %d (%d~%d)",
					size, width, height, numchar, first, last);
		}
		else {
			syslog(LOG_INFO, "Proportional font:\r\n  size: %d  width: %d~%d  height: %d  characters: %d (%d~%d)\n",
					size, pminwidth, pmaxwidth, height, numchar, first, last);
		}
	}
	return 1;
}

/*
 * Proportional fonts
 *
 */

static uint8_t getMaxWidth(void) {
	uint16_t tempPtr = 4; // point at first char data
	uint8_t cc, cw, ch, w = 0;
	do {
		cc = cfont.font[tempPtr++];
		tempPtr++;
		cw = cfont.font[tempPtr++];
		ch = cfont.font[tempPtr++];
		tempPtr += 2;
		if (cc != 0xFF) {
			if (cw != 0) {
				if (cw > w)
					w = cw;
				// packed bits
				tempPtr += (((cw * ch) - 1) / 8) + 1;
			}
		}
	} while (cc != 0xFF);

	return w;
}

static int rotatePropChar(int x, int y, int offset, int color, int fill) {
	uint8_t ch = 0;
	double radian = gdisplay_get_rotation() * 0.0175;
	float cos_radian = cos(radian);
	float sin_radian = sin(radian);

	uint8_t mask = 0x80;

	gdisplay_begin();
	for (int j = 0; j < fontChar.height; j++) {
		for (int i = 0; i < fontChar.width; i++) {
			if (((i + (j * fontChar.width)) % 8) == 0) {
				mask = 0x80;
				ch = cfont.font[fontChar.dataPtr++];
			}

			int newX = (int) (x
					+ (((offset + i) * cos_radian)
							- ((j + fontChar.adjYOffset) * sin_radian)));
			int newY = (int) (y
					+ (((j + fontChar.adjYOffset) * cos_radian)
							+ ((offset + i) * sin_radian)));

			if ((ch & mask) != 0)
				gdisplay_set_pixel(newX, newY, color);
			else if (!gdisplay_get_transparency())
				gdisplay_set_pixel(newX, newY, fill);

			mask >>= 1;
		}
	}
	gdisplay_end();

	return fontChar.xDelta + 1;
}

static int printProportionalChar(int x, int y, int color, int fill) {
	uint8_t i, j, ch = 0;
	uint16_t cx, cy;

	gdisplay_begin();

	// fill background if not transparent background
	if (!gdisplay_get_transparency()) {
		gdisplay_rect_fill(x, y, fontChar.xDelta + 1, cfont.y_size, fill, fill);
	}

	// draw Glyph
	uint8_t mask = 0x80;
	for (j = 0; j < fontChar.height; j++) {
		for (i = 0; i < fontChar.width; i++) {
			if (((i + (j * fontChar.width)) % 8) == 0) {
				mask = 0x80;
				ch = cfont.font[fontChar.dataPtr++];
			}

			if ((ch & mask) != 0) {
				cx = (uint16_t) (x + fontChar.xOffset + i);
				cy = (uint16_t) (y + j + fontChar.adjYOffset);

				gdisplay_set_pixel(cx, cy, color);
			}
			mask >>= 1;
		}
	}

	gdisplay_end();

	return fontChar.xDelta;
}

static int getCharPtr(uint8_t c) {
  uint16_t tempPtr = 4; // point at first char data

  do {
    fontChar.charCode = cfont.font[tempPtr++];
    fontChar.adjYOffset = cfont.font[tempPtr++];
    fontChar.width = cfont.font[tempPtr++];
    fontChar.height = cfont.font[tempPtr++];
    fontChar.xOffset = cfont.font[tempPtr++];
    fontChar.xOffset = fontChar.xOffset < 0x80 ? fontChar.xOffset : (0x100 - fontChar.xOffset);
    fontChar.xDelta = cfont.font[tempPtr++];

    if (c != fontChar.charCode && fontChar.charCode != 0xFF) {
      if (fontChar.width != 0) {
        // packed bits
        tempPtr += (((fontChar.width * fontChar.height)-1) / 8) + 1;
      }
    }
  } while (c != fontChar.charCode && fontChar.charCode != 0xFF);

  fontChar.dataPtr = tempPtr;
  if (c == fontChar.charCode) {
    if (gdisplay_get_force_fixed() > 0) {
      // fix width & offset for forced fixed width
      fontChar.xDelta = cfont.numchars;
      fontChar.xOffset = (fontChar.xDelta - fontChar.width) / 2;
    }
  }

  if (fontChar.charCode != 0xFF) return 1;
  else return 0;
}


/*
 * Fixed font
 *
 */

static void printChar(uint8_t c, int x, int y, int color, int fill) {
	uint8_t i, j, ch, fz, mask;
	uint16_t k, temp, cx, cy;

	gdisplay_begin();

	if (cfont.bitmap == 3) {
		temp = (c - cfont.offset) * cfont.x_size;

		if (!gdisplay_get_transparency()) {
			gdisplay_rect_fill(x, y, cfont.x_size, cfont.y_size, fill, fill);
		}

		for (i = 0; i < cfont.x_size; i++) {
			ch = cfont.font[temp + i];
			mask = 0x01;
			for (j = 0; j < cfont.y_size; j++) {
				if (ch & mask) {
					cx = (uint16_t) (x + i);
					cy = (uint16_t) (y + j);
					gdisplay_set_pixel(cx, cy, color);
				}
				mask = mask << 1;
			}
		}
	} else {
		// fz = bytes per char row
		fz = cfont.x_size / 8;
		if (cfont.x_size % 8)
			fz++;

		// get char address
		temp = ((c - cfont.offset) * ((fz) * cfont.y_size)) + 4;

		// fill background if not transparent background
		if (!gdisplay_get_transparency()) {
			gdisplay_rect_fill(x, y, cfont.x_size, cfont.y_size, fill, fill);
		}

		for (j = 0; j < cfont.y_size; j++) {
			for (k = 0; k < fz; k++) {
				ch = cfont.font[temp + k];
				mask = 0x80;
				for (i = 0; i < 8; i++) {
					if ((ch & mask) != 0) {
						cx = (uint16_t) (x + i + (k * 8));
						cy = (uint16_t) (y + j);

						gdisplay_set_pixel(cx, cy, color);
					}
					mask >>= 1;
				}
			}
			temp += (fz);
		}
	}

	gdisplay_end();
}

static void rotateChar(uint8_t c, int x, int y, int pos, int color, int fill) {
	uint8_t i, j, ch, fz, mask;
	uint16_t temp;
	int newx, newy;
	double radian = gdisplay_get_rotation() * 0.0175;
	float cos_radian = cos(radian);
	float sin_radian = sin(radian);
	int zz;

	if (cfont.x_size < 8)
		fz = cfont.x_size;
	else
		fz = cfont.x_size / 8;
	temp = ((c - cfont.offset) * ((fz) * cfont.y_size)) + 4;

	gdisplay_begin();
	for (j = 0; j < cfont.y_size; j++) {
		for (zz = 0; zz < (fz); zz++) {
			ch = cfont.font[temp + zz];
			mask = 0x80;
			for (i = 0; i < 8; i++) {
				newx = (int) (x
						+ (((i + (zz * 8) + (pos * cfont.x_size)) * cos_radian)
								- ((j) * sin_radian)));
				newy = (int) (y
						+ (((j) * cos_radian)
								+ ((i + (zz * 8) + (pos * cfont.x_size))
										* sin_radian)));

				if ((ch & mask) != 0)
					gdisplay_set_pixel(newx, newy, color);
				else if (!gdisplay_get_transparency())
					gdisplay_set_pixel(newx, newy, fill);
				mask >>= 1;
			}
		}
		temp += (fz);
	}
	gdisplay_end();

	// calculate x,y for the next char
	gdisplay_set_cursor(
		(int) (x + ((pos + 1) * cfont.x_size * cos_radian)),
		(int) (y + ((pos + 1) * cfont.x_size * sin_radian))
	);
}

static int getStringWidth(char* str) {
  // is it 7-segment font?
  if (cfont.bitmap == 2) return ((2 * (2 * cfont.y_size + 1)) + cfont.x_size) * strlen(str);

  // is it a fixed width font?
  if (cfont.x_size != 0) return strlen(str) * cfont.x_size;
  else {
    // calculate the string width
    char* tempStrptr = str;
    int strWidth = 0;
    while (*tempStrptr != 0) {
      if (getCharPtr(*tempStrptr++)) strWidth += (fontChar.xDelta + 1);
    }
    return strWidth;
  }
}

/*
 * Operation functions
 */
driver_error_t *gdisplay_set_font(uint8_t font, const char *file) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	cfont.font = NULL;

	if (font == FONT_7SEG) {
		cfont.bitmap = 2;
		cfont.x_size = 6;
		cfont.y_size = 1;
		cfont.offset = 0;
		cfont.font = (uint8_t *)font_bcd;
		cfont.color  = 0;
	} else if (font == LCD_FONT) {
		cfont.bitmap = 3;
		cfont.x_size = 5;
		cfont.y_size = 8;
		cfont.offset = 32;
		cfont.font = (uint8_t *)lcd_5;
		cfont.numchars = 96;
	} else {
		if (font == USER_FONT) {
		if (load_file_font(file, 0) == 0) cfont.font = tft_DefaultFont;
			else cfont.font = userfont;
		}
		else if (font == DEJAVU18_FONT) cfont.font = tft_Dejavu18;
		else if (font == DEJAVU24_FONT) cfont.font = tft_Dejavu24;
		else if (font == UBUNTU16_FONT) cfont.font = tft_Ubuntu16;
		else if (font == COMIC24_FONT) cfont.font = tft_Comic24;
		else if (font == MINYA24_FONT) cfont.font = tft_minya24;
		else if (font == TOONEY32_FONT) cfont.font = tft_tooney32;
		else cfont.font = tft_DefaultFont;

		cfont.bitmap = 1;
		cfont.x_size = cfont.font[0];
		cfont.y_size = cfont.font[1];
		cfont.offset = cfont.font[2];
		if (cfont.x_size != 0) cfont.numchars = cfont.font[3];
		else cfont.numchars = getMaxWidth();
	}

	return NULL;
}

driver_error_t *gdisplay_get_font_height(int *height) {
	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	if (cfont.bitmap == 1) {
		// Bitmap font
		*height = cfont.y_size;
	} else if (cfont.bitmap == 2) {
		// 7-segment font
		*height = (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size);
	} else {
		*height = 0;
	}

	return NULL;
}

driver_error_t *gdisplay_print(int x, int y, char *st, int color, int fill) {
	int stl, i, tmpw, tmph, fh;
	uint8_t ch;

	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	if (cfont.bitmap == 0) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_INVALID_FONT, NULL);
	}

	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	// for rotated string x cannot be RIGHT or CENTER
	if ((gdisplay_get_rotation() != 0) && ((x < -2) || (y < -2)))
		return NULL;

	stl = strlen(st); // number of characters in string to print

	// set CENTER or RIGHT possition
	tmpw = getStringWidth(st);
	fh = cfont.y_size; // font height
	if ((cfont.x_size != 0) && (cfont.bitmap == 2)) {
		// 7-segment font
		fh = (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size); // character height
	}

	if (x == RIGHT)
		x = caps->width - tmpw - 2;
	if (x == CENTER)
		x = (caps->width - tmpw - 2) / 2;
	if (y == BOTTOM)
		y = caps->height - fh - 2;
	if (y == CENTER)
		y = (caps->height - (fh / 2) - 2) / 2;

	int cursorx = x;
	int cursory = y;

	int offset = gdisplay_get_offset();

	tmph = cfont.y_size; // font height
	// for non-proportional fonts, char width is the same for all chars
	if (cfont.x_size != 0) {
		if (cfont.bitmap == 2) { // 7-segment font
			tmpw = (2 * (2 * cfont.y_size + 1)) + cfont.x_size; // character width
			tmph = (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size); // character height
		} else
			tmpw = cfont.x_size;

		if (cfont.bitmap == 3) {
			tmpw++;
			tmph++;
		}
	}

	gdisplay_begin();

	// adjust y position
	for (i = 0; i < stl; i++) {
		ch = *st++; // get char

		if (cfont.x_size == 0) {
			// for proportional font get char width
			if (getCharPtr(ch))
				tmpw = fontChar.xDelta;
		}

		if (ch == 0x0D) { // === '\r', erase to eol ====
			if ((!gdisplay_get_transparency()) && (gdisplay_get_rotation() == 0))
			gdisplay_rect_fill(cursorx, cursory, caps->width - cursorx, tmph, fill, fill);
		}

		else if (ch == 0x0A) { // ==== '\n', new line ====
			if (cfont.bitmap == 1) {
				cursory += tmph;
				if (cursory > (caps->width - 1 - tmph))
					break;

				cursorx = 0;
			}
		} else { // ==== other characters ====
			// check if character can be displayed in the current line
			if ((cursorx + tmpw) > (caps->width)) {
				if (gdisplay_get_wrap() == 0)
					break;
				cursory += tmph;
				if (cursory > (caps->height - tmph - 1))
					break;
				cursorx = 0;
			}

			// Let's print the character
			if (cfont.x_size == 0) {
				// == proportional font
				if (gdisplay_get_rotation() == 0) {
					cursorx += printProportionalChar(cursorx, cursory, color, fill) + 1;
				} else {
					offset += rotatePropChar(x, y, offset, color, fill);
					gdisplay_set_offset(offset);
				}
			}
			// == fixed font
			else {
				if ((cfont.bitmap == 1) || (cfont.bitmap == 3)) {
					if ((ch < cfont.offset)
							|| ((ch - cfont.offset) > cfont.numchars))
						ch = cfont.offset;
					if (gdisplay_get_rotation() == 0) {
						printChar(ch, cursorx, cursory, color, fill);
						cursorx += tmpw;
					} else
						rotateChar(ch, x, y, i, color, fill);
				} else if (cfont.bitmap == 2) { // 7-seg font
					gdisplay_draw7seg(cursorx, cursory, ch, cfont.y_size, cfont.x_size, color, fill, cfont.offset);
					cursorx += (tmpw + 2);
				}
			}
		}
	}

	gdisplay_end();

	gdisplay_set_cursor(cursorx, cursory);

	return NULL;
}

driver_error_t *gdisplay_get_font_size(int *w, int *h) {
	if (cfont.bitmap == 1) {
		if (cfont.x_size != 0)
			*w = cfont.x_size;
		else
			*w = getMaxWidth();

		*h = cfont.y_size;
	} else if (cfont.bitmap == 2) {
		*w = (2 * (2 * cfont.y_size + 1)) + cfont.x_size;
		*h = (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size);
	} else if (cfont.bitmap == 3) {
		*w = cfont.x_size;
		*h = cfont.y_size;
	} else {
		*w = 0;
		*h = 0;
	}

	return NULL;
}

driver_error_t *gdisplay_string_pos(int x, int y, const char *buf, int *posx, int *posy, int *posw, int *posh) {
	int x1, y1, x2, y2;
	int cx, cy;
	int w, h;

	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	if (cfont.bitmap == 0) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_INVALID_FONT, NULL);
	}

	// for rotated string x cannot be RIGHT or CENTER
	if ((gdisplay_get_rotation() != 0) && (x < -2)) {
		*posx = -1;
		*posy = -1;

		return NULL;
	}

	// Get cursor
	gdisplay_get_cursor(&cx, &cy);

	if ((x != LASTX) || (y != LASTY))
		gdisplay_set_offset(0);
	if (x == LASTX)
		x = cx;
	if (y == LASTY)
		y = cy;

	// Get clip window
	gdisplay_get_clip_window(&x1,&y1,&x2,&y2);

	w = getStringWidth((char*) buf);
	h = cfont.y_size; // font height
	if ((cfont.x_size != 0) && (cfont.bitmap == 2)) {
		// 7-segment font
		h = (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size); // character height
	}

	if (x == RIGHT)
		x = x2 - w - 1;
	if (x == CENTER)
		x = (x2 - w - 1) / 2;
	if (y == BOTTOM)
		y = y2 - h - 1;
	if (y == CENTER)
		y = (y2 - (h / 2) - 1) / 2;
	if (x < x1)
		x = x1;
	if (y < y1)
		y = y1;

	if ((y + h - 1) > y2) {
		*posx = -1;
		*posy = -1;

		return NULL;
	}

	*posx = x;
	*posy = y;
	*posw = w;
	*posh = h;

	return NULL;
}

#endif

/*
 * Lua RTOS, image management functions
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

#ifndef GDISPLAY_IMAGE_H_
#define GDISPLAY_IMAGE_H_

#include <sys/driver.h>

typedef struct {
	FILE *fhndl;		// File handler for input function
    uint16_t x;			// image top left point X position
    uint16_t y;			// image top left point Y position
    uint8_t *membuff;	// memory buffer containing the image
    uint32_t bufsize;	// size of the memory buffer
    uint32_t bufptr;	// memory buffer current possition
} JPGIODEV;

typedef enum {
	UNKNOWImage,
	BMPImage,
	JPGImage
} image_type;

driver_error_t *gdisplay_image_type(const char *fname, image_type *type);
driver_error_t *gdisplay_image_bmp(int x, int y, const char *fname);
driver_error_t *gdisplay_image_jpg(int x, int y, int8_t maxscale, const char *fname);
driver_error_t *gdisplay_image_raw(int x, int y, int xsize, int ysize, const char *fname);

#endif

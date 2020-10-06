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
 * Lua RTOS, bmp image management functions
 *
 */

/*
 *
 * This source code is inspired and takes code from TFT driver for Lua RTOS, authored by
 * Boris Lovošević.
 *
 * https://github.com/loboris/Lua-RTOS-ESP32-lobo/tree/master/components/lua_rtos/Lua/modules/screen
 *
 * Main changes:
 *   - Added support for BMP 1-bit
 *   - Added support for BMP 8-bit
 *   - Added sanity checks
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

#include <gdisplay/gdisplay.h>

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <sys/stat.h>

// BMP file header
typedef struct __attribute__((packed)) {
   uint16_t type;                 	  /* Magic identifier            */
   uint32_t file_size;                /* File size in bytes          */
   uint16_t reserved1, reserved2;
   uint32_t offset;                   /* Offset to image data, bytes */
   uint32_t header_size;          	  /* Header size in bytes        */
   int32_t  width,height;             /* Width and height of image   */
   uint16_t planes;       			  /* Number of colour planes     */
   uint16_t bits;         			  /* Bits per pixel              */
   uint32_t compression;        	  /* Compression type            */
   uint32_t imagesize;          	  /* Image size in bytes         */
   int32_t  xresolution,yresolution;  /* Pixels per meter            */
   uint32_t ncolours;         		  /* Number of colours           */
   uint32_t importantcolours;   	  /* Important colours           */
} bmp_header_t;

// Current palette
static uint8_t *palette = NULL;

/*
 * Operation functions
 */
driver_error_t *gdisplay_image_bmp(int x, int y, const char *fname) {
	FILE *fhndl = 0;
	uint8_t *buf = NULL;
	uint32_t xrd = 0;
	bmp_header_t header;

	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

    // Allocate buffer for reading one line of display data
    buf = malloc(caps->width * 3);
    if (!buf) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    fhndl = fopen(fname, "r");
	if (!fhndl) {
		free(buf);
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, strerror(errno));
	}

    xrd = fread(&header, sizeof(bmp_header_t), 1, fhndl);  // read header
	if (xrd != 1) {
		free(buf);
		fclose(fhndl);
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, "reading header");
	}

	// Sanity checks
	if (header.type != 0x4d42) {
		free(buf);
		fclose(fhndl);
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, "unknown signature");
	}

	if ((header.header_size != 40) && (header.header_size != 108)) {
		free(buf);
		fclose(fhndl);
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, "bad header size");
	}

	if (header.planes != 1) {
		free(buf);
		fclose(fhndl);
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, "only 1 color plane supported");
	}

	if ((header.bits != 1) && (header.bits != 8) && (header.bits != 24)) {
		free(buf);
		fclose(fhndl);
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, "only 1, 8 and 24 bits per pixel supported");
	}

	if (header.compression != 0) {
		free(buf);
		fclose(fhndl);
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, "compression not supported");
	}

	if (caps->bytes_per_pixel == 0) {
		if (header.bits != 1) {
			free(buf);
			fclose(fhndl);
			return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, "display only support 2 colors");
		}
	}

	// Load palette
	if (header.bits < 24) {
		int palette_size = 4 * (header.ncolours?header.ncolours:2 << (header.bits - 1));

		palette = malloc(palette_size);
		if (!palette) {
			free(buf);
			fclose(fhndl);
			return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_NOT_ENOUGH_MEMORY, "palette");
		}

		if (fseek(fhndl, header.offset - palette_size, SEEK_SET) != 0) {
			free(buf);
			fclose(fhndl);
			return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, strerror(errno));
		}

		int rd = fread(palette, 1, palette_size, fhndl);
		if (rd != palette_size) {
			free(buf);
			fclose(fhndl);
			return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, strerror(errno));
		}
	}

	// Adjust position
	if (x == CENTER)
		x = (caps->width - header.width) / 2;
	else
		if (x == RIGHT) x = (caps->width - header.width);

	if (x < 0) x = 0;

	if (y == CENTER) y = (caps->height - header.height) / 2;
	else if (y == BOTTOM) y = (caps->height - header.height);
	if (y < 0) y = 0;

	// Crop to display width
	int xend;
	if ((x+header.width) > caps->width) xend = caps->width-1;
	else xend = x+header.width-1;
	int disp_xsize = xend-x+1;
	if ((disp_xsize <= 1) || (y >= caps->height)) {
		free(buf);
		fclose(fhndl);

		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, "image out of screen");
	}

	int i,j,k;
	uint32_t color;

	gdisplay_begin();

	while (header.height > 0) {
		j = 0;

		if (header.bits == 1) {
			// Position at line start
			// ** BMP images are stored in file from LAST to FIRST line
			//    so we have to read from the end line first
			if (fseek(fhndl, header.offset+(((header.height-1)*((header.width + (8 - 1)) / 8))), SEEK_SET) != 0) break;

			// ** read one image line from file and send to display **
			// read only the part of image line which can be shown on screen
			xrd = fread(buf, 1, (disp_xsize + (8 - 1)) / 8, fhndl);  // read line from file
			if (xrd != ((disp_xsize + (8 - 1)) / 8)) {
				break;
			}

			for (i=0;i < xrd;i += 1) {
				for(k=0;k < 8;k++) {
					if ((buf[i] & (1 << (7-k))) == 0) {
						gdisplay_rgb_to_color(palette[2], palette[1], palette[0], &color);
					} else {
						gdisplay_rgb_to_color(palette[6], palette[5], palette[4], &color);
					}

					// Set pixel
					gdisplay_set_pixel(x + j, y, color);

					j++;
				}
			}
		} else if (header.bits == 8) {
			// Position at line start
			// ** BMP images are stored in file from LAST to FIRST line
			//    so we have to read from the end line first
			if (fseek(fhndl, header.offset+((header.height-1)*(header.width)), SEEK_SET) != 0) break;

			// ** read one image line from file and send to display **
			// read only the part of image line which can be shown on screen
			xrd = fread(buf, 1, disp_xsize, fhndl);  // read line from file
			if (xrd != (disp_xsize)) {
				break;
			}

			for (i=0;i < xrd;i += 1) {
				gdisplay_rgb_to_color(palette[buf[i] * 4 + 2], palette[buf[i] * 4 + 1], palette[buf[i] * 4], &color);

				// Set pixel
				gdisplay_set_pixel(x + i, y, color);
			}
		} else if (header.bits == 24) {
			// Position at line start
			// ** BMP images are stored in file from LAST to FIRST line
			//    so we have to read from the end line first
			if (fseek(fhndl, header.offset+((header.height-1)*(header.width*3)), SEEK_SET) != 0) break;

			// ** read one image line from file and send to display **
			// read only the part of image line which can be shown on screen
			xrd = fread(buf, 1, disp_xsize*3, fhndl);  // read line from file
			if (xrd != (disp_xsize*3)) {
				break;
			}

			for (i=0;i < xrd;i += 3) {
				// Convert color to display color
				// Color is RGB888 / BYTE ORDER B8-G8-R8
				gdisplay_rgb_to_color((int)buf[i+2], (int)buf[i+1], (int)buf[i], &color);

				// Set pixel
				gdisplay_set_pixel(x + j, y, color);
				j++;
			}
		}

		y++;	// next image line
		if (y >= caps->height) break;
		header.height--;
	}

	gdisplay_end();

	if (palette) {
		free(palette);
		palette = NULL;
	}

	free(buf);
	fclose(fhndl);

	return NULL;
}

#endif

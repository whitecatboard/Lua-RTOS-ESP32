/*
 * Lua RTOS, jpg image management functions
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
 *
 * This source code is inspired and takes code from TFT driver for Lua RTOS, authored by
 * Boris Lovošević.
 *
 * https://github.com/loboris/Lua-RTOS-ESP32-lobo/tree/master/components/lua_rtos/Lua/modules/screen
 *
 * Main changes:
 *   - Added sanity checks
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

#include <gdisplay/gdisplay.h>

#include "integer.h"
#include "tjpgd.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <sys/stat.h>

static UINT tjd_input (
	JDEC* jd,		// Decompression object
	BYTE* buff,		// Pointer to the read buffer (NULL:skip)
	UINT nd			// Number of bytes to read/skip from input stream
)
{
	int rb = 0;

	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;

	if (buff) {	// Read nd bytes from the input strem
		rb = fread(buff, 1, nd, dev->fhndl);
		return rb;	// Returns actual number of bytes read
	}
	else {	// Remove nd bytes from the input stream
		if (fseek(dev->fhndl, nd, SEEK_CUR) >= 0) {
			return nd;
		} else {
			return 0;
		}
	}
}

static UINT tjd_buf_input (
	JDEC* jd,		// Decompression object
	BYTE* buff,		// Pointer to the read buffer (NULL:skip)
	UINT nd			// Number of bytes to read/skip from input stream
)
{
	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;
	if (!dev->membuff) return 0;
	if (dev->bufptr >= (dev->bufsize + 2)) return 0; // end of stream

	if ((dev->bufptr + nd) > (dev->bufsize + 2)) nd = (dev->bufsize + 2) - dev->bufptr;

	if (buff) {	// Read nd bytes from the input strem
		memcpy(buff, dev->membuff + dev->bufptr, nd);
		dev->bufptr += nd;
		return nd;	// Returns number of bytes read
	} else {	// Remove nd bytes from the input stream
		dev->bufptr += nd;
		return nd;
	}
}

static UINT tjd_output (
	JDEC* jd,		// Decompression object of current session
	void* bitmap,	// Bitmap data to be output
	JRECT* rect		// Rectangular region to output
)
{
	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;

	// ** Put the rectangular into the display device **
	uint16_t x;
	uint16_t y;
	uint32_t color;
	uint8_t *src = (uint8_t*)bitmap;
	uint16_t left = rect->left + dev->x;
	uint16_t top = rect->top + dev->y;
	uint16_t right = rect->right + dev->x;
	uint16_t bottom = rect->bottom + dev->y;

	gdisplay_begin();

	for (y = top; y <= bottom; y++) {
		for (x = left; x <= right; x++) {
			// Convert color to display color
			gdisplay_rgb_to_color(*src, *(src + 1), *(src + 2), &color);

			// Clip to display area
			gdisplay_set_pixel(x, y, color);

			src += 3;
		}
	}

	gdisplay_end();

	return 1;	// Continue to decompression
}

driver_error_t *gdisplay_image_jpg(int x, int y, int8_t maxscale, const char *fname) {
	char *work;				// Pointer to the working buffer (must be 4-byte aligned)
	UINT sz_work = 3800;	// Size of the working buffer (must be power of 2)
	JDEC jd;				// Decompression object (70 bytes)
	JRESULT rc;
	BYTE scale = 0;
	uint8_t radj = 0;
	uint8_t badj = 0;
	JPGIODEV dev;

	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}
	if ((maxscale < 0) || (maxscale > 3)) maxscale = 3;

	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	if (caps->bytes_per_pixel == 0) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, "display only support 2 colors");
	}

	// Image from file
    dev.membuff = NULL;
    dev.bufsize = 0;

    dev.fhndl = fopen(fname, "r");
    if (!dev.fhndl) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, strerror(errno));
    }

	// Adjust position
	if ((x < 0) && (x != CENTER) && (x != RIGHT)) x = 0;
	if ((y < 0) && (y != CENTER) && (y != BOTTOM)) y = 0;
	if (x > (caps->width-5)) x = caps->width - 5;
	if (y > (caps->height-5)) y = caps->height - 5;

	work = malloc(sz_work);
	if (!work) {
		fclose(dev.fhndl);
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	gdisplay_begin();

	if (dev.membuff) {
		rc = jd_prepare(&jd, tjd_buf_input, (void *)work, sz_work, &dev);
	}
	else {
		rc = jd_prepare(&jd, tjd_input, (void *)work, sz_work, &dev);
	}

	if (rc == JDR_OK) {
		if (x == CENTER) {
			x = caps->width - (jd.width >> scale);
			if (x < 0) {
				if (maxscale) {
					for (scale = 0; scale <= maxscale; scale++) {
						if (((jd.width >> scale) <= (caps->width)) && ((jd.height >> scale) <= (caps->height))) break;
						if (scale == maxscale) break;
					}
					x = caps->width - (jd.width >> scale);
					if (x < 0) x = 0;
					else x >>= 1;
					maxscale = 0;
				}
				else x = 0;
			}
			else x >>= 1;
		}
		if (y == CENTER) {
			y = caps->height - (jd.height >> scale);
			if (y < 0) {
				if (maxscale) {
					for (scale = 0; scale <= maxscale; scale++) {
						if (((jd.width >> scale) <= (caps->width)) && ((jd.height >> scale) <= (caps->height))) break;
						if (scale == maxscale) break;
					}
					y = caps->height - (jd.height >> scale);
					if (y < 0) y = 0;
					else y >>= 1;
					maxscale = 0;
				}
				else y = 0;
			}
			else y >>= 1;
		}
		if (x == RIGHT) {
			x = 0;
			radj = 1;
		}
		if (y == BOTTOM) {
			y = 0;
			badj = 1;
		}
		// Determine scale factor
		if (maxscale) {
			for (scale = 0; scale <= maxscale; scale++) {
				if (((jd.width >> scale) <= (caps->width-x)) && ((jd.height >> scale) <= (caps->height-y))) break;
				if (scale == maxscale) break;
			}
		}

		if (radj) {
			x = caps->width - (jd.width >> scale);
			if (x < 0) x = 0;
		}
		if (badj) {
			y = caps->height - (jd.height >> scale);
			if (y < 0) y = 0;
		}
		dev.x = x;
		dev.y = y;

		// Start to decompress the JPEG file
		rc = jd_decomp(&jd, tjd_output, scale);
		if (rc != JDR_OK) {
			fclose(dev.fhndl);
			free(work);
			if (dev.membuff) free(dev.membuff);
			return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMG_PROCESSING_ERROR, "decompression error");
		}
	}
	else {
		fclose(dev.fhndl);
		free(work);
		if (dev.membuff) free(dev.membuff);
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMG_PROCESSING_ERROR, "prepare error");
	}

	free(work);  // free work buffer

	gdisplay_end();

    if (dev.fhndl) fclose(dev.fhndl);  // close input file
    if (dev.membuff) free(dev.membuff);

    return NULL;
}

#endif

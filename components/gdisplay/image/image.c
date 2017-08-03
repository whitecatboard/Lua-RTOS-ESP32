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

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

#include <gdisplay/gdisplay.h>
#include "primitives.h"
#include "image.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <sys/stat.h>

driver_error_t *gdisplay_image_type(const char *fname, image_type *type) {
	FILE *f;
	struct stat sb;
	uint8_t buffer[2];
	int rd;

	// Sanity checks
	if (!gdisplay_is_init()) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IS_NOT_SETUP, "init display first");
	}

	if (stat(fname, &sb) != 0) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, strerror(errno));
	}

    f = fopen(fname, "r");
	if (!f) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, strerror(errno));
	}

	// Read
    rd = fread(&buffer, 1, sizeof(buffer), f);
	if (rd != sizeof(buffer)) {
		fclose(f);
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_IMAGE, "reading header");
	}

	if ((buffer[0] == 0x42) && (buffer[1] == 0x4d)) {
		*type = BMPImage;
	} else if ((buffer[0] == 0xff) && (buffer[1] == 0xd8)) {
		*type = JPGImage;
	} else {
		*type = UNKNOWImage;
	}

	fclose(f);

	return NULL;

}

#endif

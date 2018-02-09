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
 * Lua RTOS, SSD1306 driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

#include "freertos/FreeRTOS.h"

#include <string.h>

#include <sys/delay.h>
#include <sys/driver.h>
#include <sys/syslog.h>

#include <gdisplay/gdisplay.h>

#include <drivers/ssd1306.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>

#include <drivers/gdisplay.h>

// SSD1306 variants
typedef struct {
	uint8_t width;
	uint8_t height;
	uint8_t compins;
	uint8_t contrast;
} ssd1306_variant_t;

static const ssd1306_variant_t variant[] = {
	{128, 32, 0x02,  0x8f}, // 128x32
	{128, 64, 0x12,  0xcf}, // 128x64
	{96,  16, 0x02,  0xaf}, // 96x16
};

// Current chipset
static uint8_t chipset;

/*
 * Helper functions
 */
static driver_error_t *ssd1306_command(int device, uint8_t command) {
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();
	driver_error_t *error;

	int transaction = I2C_TRANSACTION_INITIALIZER;
	uint8_t buff[2] = {0x00, command};

	error = i2c_start(device, &transaction);if (error) return error;
	error = i2c_write_address(device, &transaction, caps->address, 0);if (error) return error;
	error = i2c_write(device, &transaction, (char *)&buff, sizeof(buff));if (error) return error;
	error = i2c_stop(device, &transaction);if (error) return error;

	return NULL;
}

/*
 * Operation functions
 */
void ssd1306_ll_clear() {
	uint8_t *buff = (uint8_t *)gdisplay_ll_get_buffer();
	uint32_t buff_size = gdisplay_ll_get_buffer_size();
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	memset(buff,0x00,sizeof(uint8_t) * buff_size);
	ssd1306_update(0,0, caps->width - 1,caps->height - 1 , buff);
}

driver_error_t *ssd1306_init(uint8_t chip, uint8_t orient, uint8_t address) {
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();
	driver_error_t *error;

	caps->addr_window = ssd1306_addr_window;
	caps->on = ssd1306_on;
	caps->off = ssd1306_off;
	caps->invert = ssd1306_invert;
	caps->orientation = ssd1306_set_orientation;
	caps->touch_get = NULL;
	caps->touch_cal = NULL;
	caps->bytes_per_pixel = 0;
	caps->rdepth = 0;
	caps->gdepth = 0;
	caps->bdepth = 0;
	caps->phys_width = variant[chip - CHIPSET_SSD1306_VARIANT_OFFSET].width;
	caps->phys_height = variant[chip - CHIPSET_SSD1306_VARIANT_OFFSET].height;
	caps->width = caps->phys_width;
	caps->height = caps->phys_height;
	caps->interface = GDisplayI2CInterface;
	caps->monochrome_white = 1;

	if (address == 0) {
		caps->address = 0x3c;
	} else{
		caps->address = address;
	}

	// Store chipset
	chipset = chip;

	// Init I2C bus
	if ((error = i2c_setup(CONFIG_LUA_RTOS_GDISPLAY_I2C, I2C_MASTER, 400000, 0, 0, &caps->device))) {
		return error;
	}

	// Init sequence
	if ((error = ssd1306_command(caps->device, SSD1306_DISPLAYOFF))) goto i2c_error;                     // 0xAE
	if ((error = ssd1306_command(caps->device, SSD1306_SETDISPLAYCLOCKDIV))) goto i2c_error;             // 0xD5
	if ((error = ssd1306_command(caps->device, 0x80))) goto i2c_error;                                   // the suggested ratio 0x80

	if ((error = ssd1306_command(caps->device, SSD1306_SETMULTIPLEX))) goto i2c_error;                   // 0xA8
	if ((error = ssd1306_command(caps->device, caps->phys_height - 1))) goto i2c_error;

	if ((error = ssd1306_command(caps->device, SSD1306_SETDISPLAYOFFSET))) goto i2c_error;               // 0xD3
	if ((error = ssd1306_command(caps->device, 0x0))) goto i2c_error;                                    // no offset
	if ((error = ssd1306_command(caps->device, SSD1306_SETSTARTLINE | 0x0))) goto i2c_error;             // line #0
	if ((error = ssd1306_command(caps->device, SSD1306_CHARGEPUMP))) goto i2c_error;                     // 0x8D

	if ((error = ssd1306_command(caps->device, 0x14))) goto i2c_error;

	if ((error = ssd1306_command(caps->device, SSD1306_MEMORYMODE))) goto i2c_error;                     // 0x20
	if ((error = ssd1306_command(caps->device, 0x00))) goto i2c_error;                                   // 0x0 act like ks0108
	if ((error = ssd1306_command(caps->device, SSD1306_SEGREMAP | 0x1))) goto i2c_error;
	if ((error = ssd1306_command(caps->device, SSD1306_COMSCANDEC))) goto i2c_error;

	if ((error = ssd1306_command(caps->device, SSD1306_SETCOMPINS))) goto i2c_error;                   // 0xDA
	if ((error = ssd1306_command(caps->device, variant[chip - CHIPSET_SSD1306_VARIANT_OFFSET].compins))) goto i2c_error;
	if ((error = ssd1306_command(caps->device, SSD1306_SETCONTRAST))) goto i2c_error;                  // 0x81
	if ((error = ssd1306_command(caps->device, variant[chip - CHIPSET_SSD1306_VARIANT_OFFSET].contrast))) goto i2c_error;

	if ((error = ssd1306_command(caps->device, SSD1306_SETPRECHARGE))) goto i2c_error;                   // 0xd9
	if ((error = ssd1306_command(caps->device, 0xF1))) goto i2c_error;
	if ((error = ssd1306_command(caps->device, SSD1306_SETVCOMDETECT))) goto i2c_error;                  // 0xDB
	if ((error = ssd1306_command(caps->device, 0x40))) goto i2c_error;
	if ((error = ssd1306_command(caps->device, SSD1306_DISPLAYALLON_RESUME))) goto i2c_error;            // 0xA4
	if ((error = ssd1306_command(caps->device, SSD1306_NORMALDISPLAY))) goto i2c_error;                  // 0xA6

	if ((error = ssd1306_command(caps->device, SSD1306_DEACTIVATE_SCROLL))) goto i2c_error;

	if ((error = ssd1306_command(caps->device, SSD1306_DISPLAYON))) goto i2c_error; // turn on

	if ((error = ssd1306_command(caps->device, SSD1306_COLUMNADDR))) goto i2c_error;
	if ((error = ssd1306_command(caps->device, 0))) goto i2c_error;  			    // Column start address (0 = reset)
	if ((error = ssd1306_command(caps->device, caps->phys_width -1))) goto i2c_error;    // Column end address   (127 = reset)

	if ((error = ssd1306_command(caps->device, SSD1306_PAGEADDR))) goto i2c_error;
	if ((error = ssd1306_command(caps->device, 0))) goto i2c_error; // Page start address (0 = reset)

	if (caps->phys_height == 64) {
		ssd1306_command(caps->device, 7); // Page end address
	} else if (caps->phys_height == 32) {
		ssd1306_command(caps->device, 3); // Page end address
	} else if (caps->phys_height == 16) {
		ssd1306_command(caps->device, 1); // Page end address
	}

	// Allocate buffer
	if (!gdisplay_ll_allocate_buffer((caps->width * caps->height) / 8)) {
		//return driver_error(GDISPLAY_DRIVER, GDISPLAY_NOT_ENOUGH_MEMORY, NULL);
	}

	ssd1306_ll_clear();

	syslog(LOG_INFO, "OLED SSD1306 at i2c%d", CONFIG_LUA_RTOS_GDISPLAY_I2C);

	ssd1306_set_orientation(orient);

	return NULL;

i2c_error:
	return error;
}

void ssd1306_addr_window(uint8_t write, int x0, int y0, int x1, int y1) {
//	gdisplay_ll_command(ssd1306_SETYADDR);
//	gdisplay_ll_command(ssd1306_SETXADDR);
}

void ssd1306_update(int x0, int y0, int x1, int y1, uint8_t *buffer) {
	uint8_t *dst = (buffer?buffer:gdisplay_ll_get_buffer());
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();
	driver_error_t *error;

	int transaction = I2C_TRANSACTION_INITIALIZER;
	uint8_t buff[1] = {0x40};

	error = i2c_start(caps->device, &transaction);if (error) goto i2c_error;
	error = i2c_write_address(caps->device, &transaction, caps->address, 0);if (error) goto i2c_error;
	error = i2c_write(caps->device, &transaction, (char *)buff, 1);if (error) goto i2c_error;
	error = i2c_write(caps->device, &transaction, (char *)dst, (caps->height * caps->width) / 8);if (error) goto i2c_error;
	error = i2c_stop(caps->device, &transaction);if (error) goto i2c_error;

	return;

i2c_error:
	return;
}

void ssd1306_set_orientation(uint8_t orientation) {
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	caps->orient = orientation;

	if ((caps->orient == LANDSCAPE) || (caps->orient == LANDSCAPE_FLIP)) {
		caps->width = caps->phys_width;
		caps->height = caps->phys_height;
	} else {
		caps->width = caps->phys_height;
		caps->height = caps->phys_width;
	}
}

void ssd1306_on() {
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	ssd1306_command(caps->device, SSD1306_DISPLAYON);
}

void ssd1306_off() {
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	ssd1306_command(caps->device, SSD1306_DISPLAYOFF);
}

void ssd1306_invert(uint8_t on) {
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	if (on) {
		ssd1306_command(caps->device, SSD1306_INVERTDISPLAY);
	} else {
		ssd1306_command(caps->device, SSD1306_NORMALDISPLAY);
	}
}

#endif

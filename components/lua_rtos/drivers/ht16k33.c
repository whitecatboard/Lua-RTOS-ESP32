/*
 * Lua RTOS, HT16K33 segment display driver
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
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

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SDISPLAY

#include "freertos/FreeRTOS.h"

#include <sdisplay/sdisplay.h>

#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/ht16k33.h>
#include <drivers/i2c.h>

static const uint16_t map[60] = {
  0b0000000000000000, // (00 ASCII 32) SP OK
  0b0001001000000000, // (01 ASCII 33) ! OK
  0b0000001000000010, // (02 ASCII 34) " NA
  0b0010110100110110, // (03 ASCII 35) # NA
  0b0001001011101101, // (04 ASCII 36) $ NA
  0b0010010000100100, // (05 ASCII 37) % NA
  0b0010100100001001, // (06 ASCII 38) & NA
  0b0000001000000000, // (07 ASCII 39) ' NA
  0b0001111000000000, // (08 ASCII 40) ( NA
  0b0011001100000000, // (09 ASCII 41) ) NA
  0b0010110111000000, // (10 ASCII 42) * NA
  0b0001001011000000, // (11 ASCII 43) + NA
  0b0100000000000000, // (12 ASCII 44) , NA
  0b0000000011000000, // (13 ASCII 45) - NA
  0b0100000000000000, // (14 ASCII 46) . OK
  0b0010010000000000, // (15 ASCII 47) / NA
  0b0010010000111111, // (16 ASCII 48) 0 OK
  0b0000000000000110, // (17 ASCII 49) 1 OK
  0b0000000011011011, // (18 ASCII 50) 2 OK
  0b0000000011001111, // (19 ASCII 51) 3 OK
  0b0000000011100110, // (20 ASCII 52) 4 OK
  0b0000000011101101, // (21 ASCII 53) 5 OK
  0b0000000011111101, // (22 ASCII 54) 6 OK
  0b0000000000000111, // (23 ASCII 55) 7 OK
  0b0000000011111111, // (24 ASCII 56) 8 OK
  0b0000000011101111, // (25 ASCII 57) 9 OK
  0b0001001000000000, // (26 ASCII 58) : NA
  0b0010001000000000, // (27 ASCII 59) ; NA
  0b0000110000000000, // (28 ASCII 60) < NA
  0b0000000011001000, // (29 ASCII 61) = NA
  0b0010000100000000, // (30 ASCII 62) > NA
  0b0001000010100011, // (31 ASCII 63) ? NA
  0b0000001010111011, // (32 ASCII 64) @ NA
  0b0000000011110111, // (33 ASCII 65) A OK
  0b0001001010001111, // (34 ASCII 66) B OK
  0b0000000000111001, // (35 ASCII 67) C OK
  0b0001001000001111, // (36 ASCII 68) D OK
  0b0000000001111001, // (37 ASCII 69) E OK
  0b0000000001110001, // (38 ASCII 70) F OK
  0b0000000010111101, // (39 ASCII 71) G OK
  0b0000000011110110, // (40 ASCII 72) H OK
  0b0001001000001001, // (41 ASCII 73) I OK
  0b0000000000011110, // (42 ASCII 74) J OK
  0b0000110001110000, // (43 ASCII 75) K OK
  0b0000000000111000, // (44 ASCII 76) L OK
  0b0000010100110110, // (45 ASCII 77) M OK
  0b0000100100110110, // (46 ASCII 78) N OK
  0b0000000000111111, // (47 ASCII 79) O OK
  0b0000000011110011, // (48 ASCII 80) P OK
  0b0000100000111111, // (49 ASCII 81) Q OK
  0b0000100011110011, // (50 ASCII 82) R OK
  0b0000000011101101, // (51 ASCII 83) S OK
  0b0001001000000001, // (52 ASCII 84) T OK
  0b0000000000111110, // (53 ASCII 85) U OK
  0b0010010000110000, // (54 ASCII 86) V OK
  0b0010100000110110, // (55 ASCII 87) W OK
  0b0010110100000000, // (56 ASCII 88) X OK
  0b0001010100000000, // (57 ASCII 89) Y OK
  0b0010010000001001, // (58 ASCII 90) Z OK
  0b0111111111111111  // (59 ASCII 91) [ USED FOR TEST ALL SEGMENTS OK
};

/*
 * Helper functions
 *
 */
driver_error_t *ht16k33_write_digit(struct sdisplay *device, uint8_t pos, uint8_t d) {
	driver_error_t *error;

	int transaction = I2C_TRANSACTION_INITIALIZER;
	uint16_t seg_data;
	uint8_t buff[3];

	if (d - 32 < 60) {
		seg_data = map[d - 32];
	} else {
		seg_data = map[' '];
	}

	buff[0] = HT16K33_COM0 + (pos << 1);
	buff[1] = seg_data & 0xff;
	buff[2] = seg_data >> 8;

	error = i2c_start(device->config.i2c.device, &transaction);if (error) return error;
	error = i2c_write_address(device->config.i2c.device, &transaction, device->config.i2c.address, 0);if (error) return error;
	error = i2c_write(device->config.i2c.device, &transaction, (char *)&buff, 3);if (error) return error;
	error = i2c_stop(device->config.i2c.device, &transaction);if (error) return error;

	return NULL;
}

/*
 * Operation functions
 *
 */
driver_error_t *ht16k33_setup(struct sdisplay *device) {
	driver_error_t *error;

	// Init I2C bus
	if ((error = i2c_setup(1, I2C_MASTER, 400000, 0, 0, &device->config.i2c.device))) {
		return error;
	}

	// Set default address id not provided
	if (device->config.i2c.address == 0) {
		device->config.i2c.address = HT16K33_ADDR;
	}

	int transaction = I2C_TRANSACTION_INITIALIZER;
	uint8_t buff[1];

	buff[0] = 0x21;

	// turn on oscillator
	error = i2c_start(device->config.i2c.device, &transaction);if (error) return error;
	error = i2c_write_address(device->config.i2c.device, &transaction, device->config.i2c.address, 0);if (error) return error;
	error = i2c_write(device->config.i2c.device, &transaction, (char *)&buff, 1);if (error) return error;
	error = i2c_stop(device->config.i2c.device, &transaction);if (error) return error;

	// Default brightness
	error = ht16k33_brightness(device, 1);if (error) return error;

	delay(100);

	// Clear display
	error = ht16k33_clear(device);if (error) return error;

	return NULL;
}

driver_error_t *ht16k33_clear(struct sdisplay *device) {
	int transaction = I2C_TRANSACTION_INITIALIZER;
	driver_error_t *error;

	// Clear
	ht16k33_write(device,"");

	// Blink off
	uint8_t buff[1];

	buff[0] = HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (HT16K33_BLINK_OFF << 1);

	error = i2c_start(device->config.i2c.device, &transaction);if (error) return error;
	error = i2c_write_address(device->config.i2c.device, &transaction, device->config.i2c.address, 0);if (error) return error;
	error = i2c_write(device->config.i2c.device, &transaction, (char *)&buff, 1);if (error) return error;
	error = i2c_stop(device->config.i2c.device, &transaction);if (error) return error;

	return error;
}

driver_error_t *ht16k33_write(struct sdisplay *device, const char *data) {
	const char *cdata = data;
	driver_error_t *error = NULL;
	uint8_t digits = device->digits;
	uint8_t pos = 0;

	while (*cdata && !error && (pos < device->digits)) {
		error = ht16k33_write_digit(device, pos, *cdata);
		cdata++;
		digits--;
		pos++;
	}

	while (digits && !error && (pos < device->digits)) {
		ht16k33_write_digit(device, pos, ' ');
		digits--;
		pos++;
	}

	return error;
}

driver_error_t *ht16k33_brightness(struct sdisplay *device, uint8_t brightness) {
	driver_error_t *error;

	// Sanity checks
	if (brightness > 15) {
		return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_INVALID_BRIGHTNESS, NULL);
	}

	device->brightness = brightness;

	int transaction = I2C_TRANSACTION_INITIALIZER;
	uint8_t buff[1];

	buff[0] = HT16K33_CMD_BRIGHTNESS | brightness;

	// turn on oscillator
	error = i2c_start(device->config.i2c.device, &transaction);if (error) return error;
	error = i2c_write_address(device->config.i2c.device, &transaction, device->config.i2c.address, 0);if (error) return error;
	error = i2c_write(device->config.i2c.device, &transaction, (char *)&buff, 1);if (error) return error;
	error = i2c_stop(device->config.i2c.device, &transaction);if (error) return error;

	return NULL;
}
#endif

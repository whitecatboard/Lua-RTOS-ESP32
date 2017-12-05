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

#ifndef HT16K33_H_
#define HT16K33_H_

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SDISPLAY

#include <sdisplay/sdisplay.h>

#define HT16K33_COM0            0x00
#define HT16K33_COM1            0x02
#define HT16K33_COM2            0x04
#define HT16K33_COM3            0x06
#define HT16K33_COM4            0x08
#define HT16K33_COM5            0x0A
#define HT16K33_COM6            0x0C
#define HT16K33_COM7            0x0E

#define HT16K33_BLINK_OFF       0x00
#define HT16K33_BLINK_2HZ       0x01
#define HT16K33_BLINK_1HZ       0x02
#define HT16K33_BLINK_HALFHZ    0x03
#define HT16K33_BLINK_CMD       0x80
#define HT16K33_CMD_BRIGHTNESS  0xE0
#define HT16K33_BLINK_DISPLAYON 0x01

/* I2C addressing 0b01110_A2_A1_A0 */
#define HT16K33_ADDR 0b01110001

driver_error_t *ht16k33_setup(struct sdisplay *device);
driver_error_t *ht16k33_clear(struct sdisplay *device);
driver_error_t *ht16k33_write(struct sdisplay *device, const char *data);
driver_error_t *ht16k33_brightness(struct sdisplay *device, uint8_t brightness);

#endif

#endif /* HT16K33_H_ */

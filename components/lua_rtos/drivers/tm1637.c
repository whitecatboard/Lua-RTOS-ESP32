/*
 * Lua RTOS, tm1637 driver
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

/*

This driver have parts of code extracted from:

http://elecfreaks.com/estore/download/EF4056-Paintcode.zip

//  Author:Frankie.Chu
//  Date:9 April,2012
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

 */

/*

This driver have parts of code extracted from:

https://github.com/avishorp/TM1637/blob/master/TM1637Display.cpp

//  Author: avishorp@gmail.com
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SDISPLAY

#include "freertos/FreeRTOS.h"

#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/tm1637.h>
#include <drivers/gpio.h>

static int tm1637_write_byte(int deviceid, uint8_t wr_data);
static void tm1637_start(int deviceid);
static void tm1637_stop(int deviceid);

static uint8_t tm1637_map(uint8_t data, uint8_t point);

//
//      A
//     ---
//  F |   | B
//     -G-
//  E |   | C
//     ---
//      D
const uint8_t char_map[] = {'0','1','2','3','4','5','6','7','8','9','A','b','C','d','E','F'};

const uint8_t map[] = {
		// XGFEDCBA
		0b00111111,    // 0
		0b00000110,    // 1
		0b01011011,    // 2
		0b01001111,    // 3
		0b01100110,    // 4
		0b01101101,    // 5
		0b01111101,    // 6
		0b00000111,    // 7
		0b01111111,    // 8
		0b01101111,    // 9
		0b01110111,    // A
		0b01111100,    // b
		0b00111001,    // C
		0b01011110,    // d
		0b01111001,    // E
		0b01110001     // F
};

// Register drivers and errors
DRIVER_REGISTER_BEGIN(TM1637,tm1637,NULL,NULL,NULL);
	DRIVER_REGISTER_ERROR(TM1637, tm1637, TimeOut, "timeout", TM1637_ERR_TIMEOUT);
DRIVER_REGISTER_END(TM1637,tm1637,NULL,NULL,NULL);

/*
 * Helper functions
 */
static int tm1637_write_byte(int deviceid, uint8_t wr_data) {
	uint8_t scl_pin = (deviceid >> 8) & 0xff;
	uint8_t sda_pin = deviceid & 0xff;

	uint8_t i,count1 = 0, count2 = 0;
	uint8_t val;

	//sent 8bit data
	for(i=0;i<8;i++) {
		gpio_pin_clr(scl_pin);
		//LSB first
		if (wr_data & 0x01)
			gpio_pin_set(sda_pin);
		else
			gpio_pin_clr(sda_pin);
		wr_data >>= 1;
		gpio_pin_set(scl_pin);
		udelay(1);
	}

	//wait for the ACK
	gpio_pin_clr(scl_pin);
	gpio_pin_set(sda_pin);
	udelay(1);
	gpio_pin_set(scl_pin);
	gpio_pin_input(sda_pin);

	gpio_pin_get(sda_pin, &val);
	while (val) {
		count1 +=1;
		count2 +=1;

		if (count1 == 200) {
			count2 +=1;

			if (count2 == 200) {
				tm1637_stop(deviceid);
				gpio_pin_output(sda_pin);
				return -1; // Timeout
			}
			gpio_pin_output(sda_pin);
			gpio_pin_clr(sda_pin);
			count1 =0;
		}
		gpio_pin_input(sda_pin);
		gpio_pin_get(sda_pin, &val);
	}

	gpio_pin_output(sda_pin);

	return 0;
}

//send start signal to TM1637
static void tm1637_start(int deviceid) {
	uint8_t scl_pin = (deviceid >> 8) & 0xff;
	uint8_t sda_pin = deviceid & 0xff;

	portDISABLE_INTERRUPTS();

	gpio_pin_set(scl_pin);
	gpio_pin_set(sda_pin);
	udelay(1);
	gpio_pin_clr(sda_pin);
	gpio_pin_clr(scl_pin);
	udelay(1);
}

//End of transmission
static void tm1637_stop(int deviceid) {
	uint8_t scl_pin = (deviceid >> 8) & 0xff;
	uint8_t sda_pin = deviceid & 0xff;

	gpio_pin_clr(scl_pin);
	gpio_pin_clr(sda_pin);
	udelay(1);
	gpio_pin_set(scl_pin);
	gpio_pin_set(sda_pin);
	udelay(1);

	portENABLE_INTERRUPTS();
}

static uint8_t tm1637_map(uint8_t data,  uint8_t point) {
	uint8_t map_data = 0, point_data = 0;
	int i;

	for(i=0;i < sizeof(map);i++) {
		if (char_map[i] == data) {
			map_data = map[i];
			break;
		}
	}

	if (point == TM1637_POINT_ON)
		point_data = 0x80;
	else
		point_data = 0;

	if (data == 0x7f)
		data = 0x00 + point_data;
	else
		data = map_data + point_data;

	return data;
}

static int tm1637_display(int deviceid, uint8_t addr,uint8_t data, uint8_t point, uint8_t brightness) {
	uint8_t map_data;

	map_data = tm1637_map(data, point);
	tm1637_start(deviceid);
	if (tm1637_write_byte(deviceid,TM1637_ADDR_FIXED) < 0) return -1;
	tm1637_stop(deviceid);
	tm1637_start(deviceid);
	if (tm1637_write_byte(deviceid,addr|0xc0) < 0) return -1;
	if (tm1637_write_byte(deviceid,map_data) < 0) return -1;
	tm1637_stop(deviceid);
	tm1637_start(deviceid);
	if (tm1637_write_byte(deviceid,0x88 + brightness) < 0) return -1;
	tm1637_stop(deviceid);

	return 0;
}

/*
 * Operation functions
 *
 */
driver_error_t *tm1637_setup(uint8_t scl, uint8_t sda, int *deviceid) {
	driver_unit_lock_error_t *lock_error = NULL;

	*deviceid = (scl << 8) | sda;

	// Lock scl
    if ((lock_error = driver_lock(TM1637_DRIVER, *deviceid, GPIO_DRIVER, scl, DRIVER_ALL_FLAGS, "SCL"))) {
    	// Revoked lock on ADC channel
    	return driver_lock_error(TM1637_DRIVER, lock_error);
    }

	// Lock sda
    if ((lock_error = driver_lock(TM1637_DRIVER, *deviceid, GPIO_DRIVER, sda, DRIVER_ALL_FLAGS, "SDA"))) {
    	// Revoked lock on ADC channel
    	return driver_lock_error(TM1637_DRIVER, lock_error);
    }

    gpio_pin_output(scl);
	gpio_pin_output(sda);


	tm1637_clear(*deviceid);

	return NULL;
}

driver_error_t *tm1637_clear(int deviceid) {
	if (tm1637_display(deviceid, 0x00,0x7f,TM1637_POINT_OFF,TM1637_BRIGHT_TYPICAL) < 0) {
		return driver_error(TM1637_DRIVER, TM1637_ERR_TIMEOUT, NULL);
	}

	if (tm1637_display(deviceid, 0x01,0x7f,TM1637_POINT_OFF,TM1637_BRIGHT_TYPICAL) < 0) {
		return driver_error(TM1637_DRIVER, TM1637_ERR_TIMEOUT, NULL);
	}

	if (tm1637_display(deviceid, 0x02,0x7f,TM1637_POINT_OFF,TM1637_BRIGHT_TYPICAL) < 0) {
		return driver_error(TM1637_DRIVER, TM1637_ERR_TIMEOUT, NULL);
	}

	if (tm1637_display(deviceid, 0x03,0x7f,TM1637_POINT_OFF,TM1637_BRIGHT_TYPICAL) < 0) {
		return driver_error(TM1637_DRIVER, TM1637_ERR_TIMEOUT, NULL);
	}

	if (tm1637_display(deviceid, 0x04,0x7f,TM1637_POINT_OFF,TM1637_BRIGHT_TYPICAL) < 0) {
		return driver_error(TM1637_DRIVER, TM1637_ERR_TIMEOUT, NULL);
	}

	return NULL;
}

driver_error_t *tm1637_write(int deviceid, const char *data, uint8_t brightness) {
	const char *cdata = data;
	uint8_t pos = 0;

	while (*cdata) {
		if (*(cdata + 1) == '.') {
			if (tm1637_display(deviceid, pos,(uint8_t)*cdata,TM1637_POINT_ON,brightness) < 0) {
				return driver_error(TM1637_DRIVER, TM1637_ERR_TIMEOUT, NULL);
			}
			cdata++;
		} else {
			if (tm1637_display(deviceid, pos,(uint8_t)*cdata,TM1637_POINT_OFF,brightness) < 0) {
				return driver_error(TM1637_DRIVER, TM1637_ERR_TIMEOUT, NULL);
			}
		}

		cdata++;
		pos++;
	}

	return NULL;
}

#endif

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
 * Lua RTOS, tm1637 driver
 *
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

#include <sdisplay/sdisplay.h>

#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/tm1637.h>
#include <drivers/gpio.h>

static int tm1637_write_byte(sdisplay_device_t *device, uint8_t wr_data);
static void tm1637_start(sdisplay_device_t *device);
static void tm1637_stop(sdisplay_device_t *device);
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

/*
 * Helper functions
 */
static int tm1637_write_byte(sdisplay_device_t *device, uint8_t wr_data) {
	uint8_t i,count1 = 0, count2 = 0;
	uint8_t val;

	//sent 8bit data
	for(i=0;i<8;i++) {
		gpio_pin_clr(device->config.wire.clk);
		//LSB first
		if (wr_data & 0x01)
			gpio_pin_set(device->config.wire.dio);
		else
			gpio_pin_clr(device->config.wire.dio);
		wr_data >>= 1;
		gpio_pin_set(device->config.wire.clk);
		udelay(1);
	}

	//wait for the ACK
	gpio_pin_clr(device->config.wire.clk);
	gpio_pin_set(device->config.wire.dio);
	udelay(1);
	gpio_pin_set(device->config.wire.clk);
	gpio_pin_input(device->config.wire.dio);

	gpio_pin_get(device->config.wire.dio, &val);
	while (val) {
		count1 +=1;
		count2 +=1;

		if (count1 == 200) {
			count2 +=1;

			if (count2 == 200) {
				tm1637_stop(device);
				gpio_pin_output(device->config.wire.dio);
				return -1; // Timeout
			}
			gpio_pin_output(device->config.wire.dio);
			gpio_pin_clr(device->config.wire.dio);
			count1 =0;
		}
		gpio_pin_input(device->config.wire.dio);
		gpio_pin_get(device->config.wire.dio, &val);
	}

	gpio_pin_output(device->config.wire.dio);

	return 0;
}

//send start signal to TM1637
static void tm1637_start(sdisplay_device_t *device) {
	portDISABLE_INTERRUPTS();

	gpio_pin_set(device->config.wire.clk);
	gpio_pin_set(device->config.wire.dio);
	udelay(1);
	gpio_pin_clr(device->config.wire.dio);
	gpio_pin_clr(device->config.wire.clk);
	udelay(1);
}

//End of transmission
static void tm1637_stop(sdisplay_device_t *device) {
	gpio_pin_clr(device->config.wire.clk);
	gpio_pin_clr(device->config.wire.dio);
	udelay(1);
	gpio_pin_set(device->config.wire.clk);
	gpio_pin_set(device->config.wire.dio);
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

static int tm1637_display(sdisplay_device_t *device, uint8_t addr,uint8_t data, uint8_t point, uint8_t brightness) {
	uint8_t map_data;

	map_data = tm1637_map(data, point);
	tm1637_start(device);
	if (tm1637_write_byte(device,TM1637_ADDR_FIXED) < 0) return -1;
	tm1637_stop(device);
	tm1637_start(device);
	if (tm1637_write_byte(device,addr|0xc0) < 0) return -1;
	if (tm1637_write_byte(device,map_data) < 0) return -1;
	tm1637_stop(device);
	tm1637_start(device);
	if (tm1637_write_byte(device,0x88 + brightness) < 0) return -1;
	tm1637_stop(device);

	return 0;
}

/*
 * Operation functions
 *
 */
driver_error_t *tm1637_setup(struct sdisplay *device) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	driver_unit_lock_error_t *lock_error = NULL;
#endif

	int clk = device->config.wire.clk;
	int dio = device->config.wire.dio;

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	// Lock clk
    if ((lock_error = driver_lock(SDISPLAY_DRIVER, device->id, GPIO_DRIVER, clk, DRIVER_ALL_FLAGS, "CLK"))) {
    	// Revoked lock on ADC channel
    	return driver_lock_error(SDISPLAY_DRIVER, lock_error);
    }

	// Lock dio
    if ((lock_error = driver_lock(SDISPLAY_DRIVER, device->id, GPIO_DRIVER, dio, DRIVER_ALL_FLAGS, "DIO"))) {
    	// Revoked lock on ADC channel
    	return driver_lock_error(SDISPLAY_DRIVER, lock_error);
    }
#endif

    gpio_pin_output(clk);
	gpio_pin_output(dio);

	device->brightness = 7;

	tm1637_clear(device);

	return NULL;
}

driver_error_t *tm1637_clear(struct sdisplay *device) {
	if (tm1637_display(device, 0x00,0x7f,TM1637_POINT_OFF,TM1637_BRIGHT_TYPICAL) < 0) {
		return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_TIMEOUT, NULL);
	}

	if (tm1637_display(device, 0x01,0x7f,TM1637_POINT_OFF,TM1637_BRIGHT_TYPICAL) < 0) {
		return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_TIMEOUT, NULL);
	}

	if (tm1637_display(device, 0x02,0x7f,TM1637_POINT_OFF,TM1637_BRIGHT_TYPICAL) < 0) {
		return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_TIMEOUT, NULL);
	}

	if (tm1637_display(device, 0x03,0x7f,TM1637_POINT_OFF,TM1637_BRIGHT_TYPICAL) < 0) {
		return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_TIMEOUT, NULL);
	}

	if (tm1637_display(device, 0x04,0x7f,TM1637_POINT_OFF,TM1637_BRIGHT_TYPICAL) < 0) {
		return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_TIMEOUT, NULL);
	}

	return NULL;
}

driver_error_t *tm1637_write(struct sdisplay *device, const char *data) {
	const char *cdata = data;
	uint8_t pos = 0;

	while (*cdata) {
		if (*(cdata + 1) == '.') {
			if (tm1637_display(device, pos,(uint8_t)*cdata,TM1637_POINT_ON,device->brightness) < 0) {
				return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_TIMEOUT, NULL);
			}
			cdata++;
		} else {
			if (tm1637_display(device, pos,(uint8_t)*cdata,TM1637_POINT_OFF,device->brightness) < 0) {
				return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_TIMEOUT, NULL);
			}
		}

		cdata++;
		pos++;
	}

	return NULL;
}

driver_error_t *tm1637_brightness(struct sdisplay *device, uint8_t brightness) {
	// Sanity checks
	if (brightness > 7) {
		return driver_error(SDISPLAY_DRIVER, SDISPLAY_ERR_INVALID_BRIGHTNESS, NULL);
	}

	device->brightness = brightness;

	return NULL;
}
#endif

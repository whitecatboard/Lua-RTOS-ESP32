

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

#ifndef TM1637_H_
#define TM1637_H_

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SDISPLAY

#include <sdisplay/sdisplay.h>

driver_error_t *tm1637_setup(struct sdisplay *device);
driver_error_t *tm1637_clear(struct sdisplay *device);
driver_error_t *tm1637_write(struct sdisplay *device, const char *data);
driver_error_t *tm1637_brightness(struct sdisplay *device, uint8_t brightness);

//************definitions for TM1637*********************
#define TM1637_ADDR_AUTO  0x40
#define TM1637_ADDR_FIXED 0x44

#define TM1637_STARTADDR  0xc0

/**** definitions for the clock point of the digit tube *******/
#define TM1637_POINT_ON   1
#define TM1637_POINT_OFF  0

/**************definitions for brightness***********************/
#define  TM1637_BRIGHT_DARKEST 0
#define  TM1637_BRIGHT_TYPICAL 2
#define  TM1637_BRIGHTEST      7

#endif

#endif /* TM1637_H_ */

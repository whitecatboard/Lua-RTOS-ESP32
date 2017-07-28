/*
 * Lua RTOS, PCF8544 driver
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

#ifndef PCD8544_H_
#define PCD8544_H_

#include "luartos.h"

#if LUA_RTOS_LUA_USE_GDISPLAY

#include <stdint.h>

#include <sys/driver.h>

#define LCDWIDTH 84
#define LCDHEIGHT 48

#define PCD8544_POWERDOWN 0x04
#define PCD8544_ENTRYMODE 0x02
#define PCD8544_EXTENDEDINSTRUCTION 0x01

#define PCD8544_DISPLAYBLANK 0x0
#define PCD8544_DISPLAYNORMAL 0x4
#define PCD8544_DISPLAYALLON 0x1
#define PCD8544_DISPLAYINVERTED 0x5

// H = 0
#define PCD8544_FUNCTIONSET 0x20
#define PCD8544_DISPLAYCONTROL 0x08
#define PCD8544_SETYADDR 0x40
#define PCD8544_SETXADDR 0x80

// H = 1
#define PCD8544_SETTEMP 0x04
#define PCD8544_SETBIAS 0x10
#define PCD8544_SETVOP 0x80

//  PCD854 errors
#define PCD8544_CANNOT_SETUP             (DRIVER_EXCEPTION_BASE(PCD8544_DRIVER_ID) |  0)
#define PCD8544_NOT_ENOUGH_MEMORY		 (DRIVER_EXCEPTION_BASE(PCD8544_DRIVER_ID) |  1)

driver_error_t *pcd8544_init(uint8_t chipset, uint8_t orientation);
void pcd8544_addr_window(uint8_t write, int x0, int y0, int x1, int y1);
void pcd8544_update(int x0, int y0, int x1, int y1, uint8_t *buffer);
void pcd8544_set_orientation(uint8_t m);
void pcd8544_on();
void pcd8544_off();
void pcd8544_invert(uint8_t on);

#endif

#endif /* PCD8544_H_ */

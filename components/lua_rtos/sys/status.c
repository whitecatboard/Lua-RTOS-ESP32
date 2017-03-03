/*
 * Lua RTOS, LuaOS status management
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

#include <sys/status.h>

uint32_t LuaOS_status[] = {0};

void IRAM_ATTR status_set(uint16_t flag) {
	LuaOS_status[(flag >> 8)] |= (1 << (flag & 0x00ff));
}

void IRAM_ATTR status_clear(uint16_t flag) {
	LuaOS_status[(flag >> 8)] &= ~(1 << (flag & 0x00ff));
}

int IRAM_ATTR status_get(uint16_t flag) {
	int value;

	value = (LuaOS_status[(flag >> 8)] & (1 << (flag & 0x00ff)));

	return value;
}


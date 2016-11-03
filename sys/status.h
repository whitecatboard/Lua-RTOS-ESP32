/*
 * Lua RTOS, Lua RTOS status management
 *
 * Copyright (C) 2015 - 2016
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

#ifndef _SYS_STATUS_H_
#define _SYS_STATUS_H_

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>

#define STATUS_SYSCALLS_INITED	       0x0000
#define STATUS_LUA_RUNNING			   0x0001
#define STATUS_LUA_INTERPRETER  	   0x0002
#define STATUS_LUA_ABORT_BOOT_SCRIPTS  0x0003
#define STATUS_LUA_HISTORY			   0x0004
#define STATUS_LUA_SHELL			   0x0005

extern uint32_t LuaOS_status[];

inline void status_set(uint16_t flag) {
	enter_critical_section();
	LuaOS_status[(flag >> 8)] |= (1 << (flag & 0x00ff));
	exit_critical_section();
}

inline void status_clear(uint16_t flag) {
	enter_critical_section();
	LuaOS_status[(flag >> 8)] &= ~(1 << (flag & 0x00ff));
	exit_critical_section();
}

inline int status_get(uint16_t flag) {
	int value;
	
	enter_critical_section();
	value = (LuaOS_status[(flag >> 8)] & (1 << (flag & 0x00ff)));
	exit_critical_section();
	
	return value;
}

#endif /* !_SYS_STATUS_H_ */

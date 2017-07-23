/*
 * Lua RTOS, timer MODULE
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

#ifndef _LUA_TMR_H
#define	_LUA_TMR_H

#include <drivers/cpu.h>

typedef enum {
	TmrHW,
	TmrSW
} tmr_type_t;

typedef struct {
	tmr_type_t type;
	int8_t unit;
	TimerHandle_t h;
} tmr_userdata;

#ifdef CPU_TIMER0
#define TMR_TMR0 {LSTRKEY(CPU_TIMER0_NAME), LINTVAL(CPU_TIMER0)},
#else
#define TMR_TMR0
#endif

#ifdef CPU_TIMER1
#define TMR_TMR1 {LSTRKEY(CPU_TIMER1_NAME), LINTVAL(CPU_TIMER1)},
#else
#define TMR_TMR1
#endif

#ifdef CPU_TIMER2
#define TMR_TMR2 {LSTRKEY(CPU_TIMER2_NAME), LINTVAL(CPU_TIMER2)},
#else
#define TMR_TMR2
#endif

#ifdef CPU_TIMER3
#define TMR_TMR3 {LSTRKEY(CPU_TIMER3_NAME), LINTVAL(CPU_TIMER3)},
#else
#define TMR_TMR3
#endif

#endif

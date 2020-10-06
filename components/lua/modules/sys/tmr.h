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
 * Lua RTOS, Lua timer module
 *
 */

#ifndef _LUA_TMR_H
#define	_LUA_TMR_H

#include "sys.h"

#include <drivers/cpu.h>

typedef enum {
	TmrHW = 1,
	TmrSW = 2
} tmr_type_t;

typedef struct {
	tmr_type_t type;
	int8_t std;
	int8_t unit;
	TimerHandle_t h;
	lua_callback_t *callback;
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

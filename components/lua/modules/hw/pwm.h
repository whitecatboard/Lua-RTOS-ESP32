/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, Lua PWM module
 *
 */

#ifndef LPWM_H
#define	LPWM_H

#include <stdint.h>

#include <drivers/cpu.h>

typedef struct {
	int8_t unit;
	int8_t channel;
} pwm_userdata;

#ifdef CPU_PWM0
#define PWM_PWM0 {LSTRKEY(CPU_PWM0_NAME), LINTVAL(CPU_PWM0)},
#else
#define PWM_PWM0
#endif

#ifdef CPU_PWM1
#define PWM_PWM1 {LSTRKEY(CPU_PWM1_NAME), LINTVAL(CPU_PWM1)},
#else
#define PWM_PWM1
#endif

#ifdef CPU_PWM_CH0
#define PWM_PWM_CH0 {LSTRKEY(CPU_PWM_CH0_NAME), LINTVAL(CPU_PWM_CH0)},
#else
#define PWM_PWM_CH0
#endif

#ifdef CPU_PWM_CH1
#define PWM_PWM_CH1 {LSTRKEY(CPU_PWM_CH1_NAME), LINTVAL(CPU_PWM_CH1)},
#else
#define PWM_PWM_CH1
#endif

#ifdef CPU_PWM_CH2
#define PWM_PWM_CH2 {LSTRKEY(CPU_PWM_CH2_NAME), LINTVAL(CPU_PWM_CH2)},
#else
#define PWM_PWM_CH2
#endif

#ifdef CPU_PWM_CH3
#define PWM_PWM_CH3 {LSTRKEY(CPU_PWM_CH3_NAME), LINTVAL(CPU_PWM_CH3)},
#else
#define PWM_PWM_CH3
#endif

#ifdef CPU_PWM_CH4
#define PWM_PWM_CH4 {LSTRKEY(CPU_PWM_CH4_NAME), LINTVAL(CPU_PWM_CH4)},
#else
#define PWM_PWM_CH4
#endif

#ifdef CPU_PWM_CH5
#define PWM_PWM_CH5 {LSTRKEY(CPU_PWM_CH5_NAME), LINTVAL(CPU_PWM_CH5)},
#else
#define PWM_PWM_CH5
#endif

#ifdef CPU_PWM_CH6
#define PWM_PWM_CH6 {LSTRKEY(CPU_PWM_CH6_NAME), LINTVAL(CPU_PWM_CH6)},
#else
#define PWM_PWM_CH6
#endif

#ifdef CPU_PWM_CH7
#define PWM_PWM_CH7 {LSTRKEY(CPU_PWM_CH7_NAME), LINTVAL(CPU_PWM_CH7)},
#else
#define PWM_PWM_CH7
#endif

#ifdef CPU_PWM_CH8
#define PWM_PWM_CH8 {LSTRKEY(CPU_PWM_CH8_NAME), LINTVAL(CPU_PWM_CH8)},
#else
#define PWM_PWM_CH8
#endif

#ifdef CPU_PWM_CH9
#define PWM_PWM_CH9 {LSTRKEY(CPU_PWM_CH9_NAME), LINTVAL(CPU_PWM_CH9)},
#else
#define PWM_PWM_CH9
#endif

#ifdef CPU_PWM_CH10
#define PWM_PWM_CH10 {LSTRKEY(CPU_PWM_CH10_NAME), LINTVAL(CPU_PWM_CH10)},
#else
#define PWM_PWM_CH10
#endif

#ifdef CPU_PWM_CH11
#define PWM_PWM_CH11 {LSTRKEY(CPU_PWM_CH11_NAME), LINTVAL(CPU_PWM_CH11)},
#else
#define PWM_PWM_CH11
#endif

#ifdef CPU_PWM_CH12
#define PWM_PWM_CH12 {LSTRKEY(CPU_PWM_CH12_NAME), LINTVAL(CPU_PWM_CH12)},
#else
#define PWM_PWM_CH12
#endif

#ifdef CPU_PWM_CH13
#define PWM_PWM_CH13 {LSTRKEY(CPU_PWM_CH13_NAME), LINTVAL(CPU_PWM_CH13)},
#else
#define PWM_PWM_CH13
#endif

#ifdef CPU_PWM_CH14
#define PWM_PWM_CH14 {LSTRKEY(CPU_PWM_CH14_NAME), LINTVAL(CPU_PWM_CH14)},
#else
#define PWM_PWM_CH14
#endif

#ifdef CPU_PWM_CH15
#define PWM_PWM_CH15 {LSTRKEY(CPU_PWM_CH15_NAME), LINTVAL(CPU_PWM_CH15)},
#else
#define PWM_PWM_CH15
#endif

#endif

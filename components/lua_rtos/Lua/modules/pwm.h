/*
 * Lua RTOS, PWM Lua module
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

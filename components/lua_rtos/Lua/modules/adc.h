/*
 * Lua RTOS, adc wrapper
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

#ifndef LADC_H
#define	LADC_H

#include "drivers/adc.h"
#include "drivers/cpu.h"

typedef struct {
    unsigned int adc;
    unsigned int chan;
} adc_userdata;

#ifdef CPU_ADC0
#define ADC_ADC0 {LSTRKEY(CPU_ADC0_NAME), LINTVAL(CPU_ADC0)},
#else
#define ADC_ADC0
#endif

#ifdef CPU_ADC1
#define ADC_ADC1 {LSTRKEY(CPU_ADC1_NAME), LINTVAL(CPU_ADC1)},
#else
#define ADC_ADC1
#endif

#ifdef CPU_ADC2
#define ADC_ADC2 {LSTRKEY(CPU_ADC2_NAME), LINTVAL(CPU_ADC2)},
#else
#define ADC_ADC2
#endif

#ifdef CPU_ADC3
#define ADC_ADC3 {LSTRKEY(CPU_ADC3_NAME), LINTVAL(CPU_ADC3)},
#else
#define ADC_ADC3
#endif

#ifdef CPU_ADC_CH0
#define ADC_ADC_CH0 {LSTRKEY(CPU_ADC_CH0_NAME), LINTVAL(CPU_ADC_CH0)},
#else
#define ADC_ADC_CH0
#endif

#ifdef CPU_ADC_CH1
#define ADC_ADC_CH1 {LSTRKEY(CPU_ADC_CH1_NAME), LINTVAL(CPU_ADC_CH1)},
#else
#define ADC_ADC_CH1
#endif

#ifdef CPU_ADC_CH2
#define ADC_ADC_CH2 {LSTRKEY(CPU_ADC_CH2_NAME), LINTVAL(CPU_ADC_CH2)},
#else
#define ADC_ADC_CH2
#endif

#ifdef CPU_ADC_CH3
#define ADC_ADC_CH3 {LSTRKEY(CPU_ADC_CH3_NAME), LINTVAL(CPU_ADC_CH3)},
#else
#define ADC_ADC_CH3
#endif

#ifdef CPU_ADC_CH4
#define ADC_ADC_CH4 {LSTRKEY(CPU_ADC_CH4_NAME), LINTVAL(CPU_ADC_CH4)},
#else
#define ADC_ADC_CH4
#endif

#ifdef CPU_ADC_CH5
#define ADC_ADC_CH5 {LSTRKEY(CPU_ADC_CH5_NAME), LINTVAL(CPU_ADC_CH5)},
#else
#define ADC_ADC_CH5
#endif

#ifdef CPU_ADC_CH6
#define ADC_ADC_CH6 {LSTRKEY(CPU_ADC_CH6_NAME), LINTVAL(CPU_ADC_CH6)},
#else
#define ADC_ADC_CH6
#endif

#ifdef CPU_ADC_CH7
#define ADC_ADC_CH7 {LSTRKEY(CPU_ADC_CH7_NAME), LINTVAL(CPU_ADC_CH7)},
#else
#define ADC_ADC_CH7
#endif

#endif	/* ADC_H */


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
 * Lua RTOS, Lua ADC module
 *
 */


#ifndef LADC_H
#define	LADC_H

#include "drivers/adc.h"
#include "drivers/cpu.h"

typedef struct {
    adc_channel_h_t h;
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

#ifdef CPU_ADC4
#define ADC_ADC4 {LSTRKEY(CPU_ADC4_NAME), LINTVAL(CPU_ADC4)},
#else
#define ADC_ADC4
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


/*
 * Lua RTOS, ADC driver
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

#ifndef ADC_H
#define	ADC_H

#include "unistd.h"

#define NADC 1

#define ADC_NOT_CONFIGURED -1
#define ADC_CHAN_NOT_CONFIGURED -2
#define ADC_NO_MEM -3
#define ADC_NOT_AVAILABLE_CHANNELS -4
#define ADC_CHANNEL_DOES_NOT_EXIST -5

#define ADCSEL_PBCLK             (0b00 << 30)
#define ADCSEL_FRC               (0b01 << 30)
#define ADCSEL_REFCLK3           (0b10 << 30)
#define ADCSEL_SYSCLK            (0b11 << 30)

#define ADC_CONCLKDIV_1	 	 (0  << 24)
#define ADC_CONCLKDIV_2	 	 (1  << 24)
#define ADC_CONCLKDIV_4	 	 (2  << 24)
#define ADC_CONCLKDIV_6	 	 (3  << 24)
#define ADC_CONCLKDIV_8	 	 (4  << 24)
#define ADC_CONCLKDIV_10	 (5  << 24)
#define ADC_CONCLKDIV_12	 (6  << 24)
#define ADC_CONCLKDIV_14	 (7  << 24)
#define ADC_CONCLKDIV_16	 (8  << 24)
#define ADC_CONCLKDIV_18	 (9  << 24)
#define ADC_CONCLKDIV_20	 (10 << 24)
#define ADC_CONCLKDIV_22	 (11 << 24)
#define ADC_CONCLKDIV_24	 (12 << 24)
#define ADC_CONCLKDIV_26	 (13 << 24)
#define ADC_CONCLKDIV_28	 (14 << 24)
#define ADC_CONCLKDIV_30	 (15 << 24)
#define ADC_CONCLKDIV_32	 (16 << 24)
#define ADC_CONCLKDIV_34	 (17 << 24)
#define ADC_CONCLKDIV_36	 (18 << 24)
#define ADC_CONCLKDIV_38	 (19 << 24)
#define ADC_CONCLKDIV_40	 (20 << 24)
#define ADC_CONCLKDIV_42	 (21 << 24)
#define ADC_CONCLKDIV_44	 (22 << 24)
#define ADC_CONCLKDIV_46	 (23 << 24)
#define ADC_CONCLKDIV_48	 (24 << 24)
#define ADC_CONCLKDIV_50	 (25 << 24)
#define ADC_CONCLKDIV_52	 (26 << 24)
#define ADC_CONCLKDIV_54	 (27 << 24)
#define ADC_CONCLKDIV_56	 (28 << 24)
#define ADC_CONCLKDIV_58	 (29 << 24)
#define ADC_CONCLKDIV_60	 (30 << 24)
#define ADC_CONCLKDIV_62	 (31 << 24)
#define ADC_CONCLKDIV_64	 (32 << 24)
#define ADC_CONCLKDIV_66	 (33 << 24)
#define ADC_CONCLKDIV_68	 (34 << 24)
#define ADC_CONCLKDIV_70	 (35 << 24)
#define ADC_CONCLKDIV_72	 (36 << 24)
#define ADC_CONCLKDIV_74	 (37 << 24)
#define ADC_CONCLKDIV_76	 (38 << 24)
#define ADC_CONCLKDIV_78	 (39 << 24)
#define ADC_CONCLKDIV_80	 (40 << 24)
#define ADC_CONCLKDIV_82	 (41 << 24)
#define ADC_CONCLKDIV_84	 (42 << 24)
#define ADC_CONCLKDIV_86	 (43 << 24)
#define ADC_CONCLKDIV_88	 (44 << 24)
#define ADC_CONCLKDIV_90	 (45 << 24)
#define ADC_CONCLKDIV_92	 (46 << 24)
#define ADC_CONCLKDIV_94	 (47 << 24)
#define ADC_CONCLKDIV_96	 (48 << 24)
#define ADC_CONCLKDIV_98	 (49 << 24)
#define ADC_CONCLKDIV_100	 (50 << 24)
#define ADC_CONCLKDIV_102	 (51 << 24)
#define ADC_CONCLKDIV_104	 (52 << 24)
#define ADC_CONCLKDIV_106	 (53 << 24)
#define ADC_CONCLKDIV_108	 (54 << 24)
#define ADC_CONCLKDIV_110	 (55 << 24)
#define ADC_CONCLKDIV_112	 (56 << 24)
#define ADC_CONCLKDIV_114	 (57 << 24)
#define ADC_CONCLKDIV_116	 (58 << 24)
#define ADC_CONCLKDIV_118	 (59 << 24)
#define ADC_CONCLKDIV_120	 (60 << 24)
#define ADC_CONCLKDIV_122	 (61 << 24)
#define ADC_CONCLKDIV_124	 (62 << 24)
#define ADC_CONCLKDIV_126	 (63 << 24)



void adc_setup(unsigned int ref_type);
void adc_start();
void adc_stop();

int adc_setup_channel(int channel);
int adc_read(int channel);

void adc_new_sample( void * pvParameter1, uint32_t ulParameter2 );
float adc_convert_to_mv(int channel, unsigned short val);

#endif	/* ADC_H */

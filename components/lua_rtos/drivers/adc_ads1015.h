/*
 * Lua RTOS, ADC ADS1015 driver
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
#ifndef _ADC_ADS1015_H
#define _ADC_ADS1015_H

#include <machine/endian.h>

#include <drivers/adc.h>
#include <sys/driver.h>

// ADS1015 adress
#define ADS1015_ADDR1 0b1001000
#define ADS1015_ADDR2 0b1001001
#define ADS1015_ADDR3 0b1001010
#define ADS1015_ADDR4 0b1001011

// Registers
#define ADS1015_CONV      0x00 ///< Conversion register
#define ADS1015_CONF      0x01 ///< Config register
#define ADS1015_LO_THRESH 0x02 ///< Lo_thresh register
#define ADS1015_HI_THRESH 0x03 ///< Hi_thresh register

/*
 * ADS1015_CONF bit values
 */

// Operational status or single-shot conversion start
#define ADS1015_CONF_STATUS_BUSY 0x00 ///< Device is currently performing a conversion
#define ADS1015_CONF_STATUS_IDLE 0x01 ///< Device is not currently performing a conversion
#define ADS1015_CONF_START_CONV  0x01 ///< tart a single conversion

// Input multiplexer configuration
#define ADS1015_CONF_AIN0_AIN1 0x00 ///< AINP = AIN0 and AINN = AIN1
#define ADS1015_CONF_AIN0_AIN3 0x01 ///< AINP = AIN0 and AINN = AIN3
#define ADS1015_CONF_AIN1_AIN3 0x02 ///< AINP = AIN1 and AINN = AIN3
#define ADS1015_CONF_AIN2_AIN3 0x03 ///< AINP = AIN2 and AINN = AIN3
#define ADS1015_CONF_AIN0_GND  0x04 ///< AINP = AIN0 and AINN = GND
#define ADS1015_CONF_AIN1_GND  0x05 ///< AINP = AIN1 and AINN = GND
#define ADS1015_CONF_AIN2_GND  0x06 ///< AINP = AIN2 and AINN = GND
#define ADS1015_CONF_AIN3_GND  0x07 ///< AINP = AIN3 and AINN = GND

// Programmable gain amplifier configuration
#define ADS1015_CONF_PGA_6144  0x00 ///< FSR = ±6.144 V
#define ADS1015_CONF_PGA_4096  0x01 ///< FSR = ±4.096 V
#define ADS1015_CONF_PGA_2048  0x02 ///< FSR = ±2.048 V
#define ADS1015_CONF_PGA_1024  0x03 ///< FSR = ±1.024 V
#define ADS1015_CONF_PGA_0512  0x04 ///< FSR = ±0.512 V
#define ADS1015_CONF_PGA_0256  0x05 ///< FSR = ±0.256 V

// Device operating mode
#define ADS1015_CONF_MODE_CONTINUOUS 0x00 ///< Continuous-conversion mode
#define ADS1015_CONF_MODE_SINGLE     0x01 ///< Single-shot mode or power-down state

// Data rate
#define ADS1015_CONF_DR_128   0x00 ///< 128 SPS
#define ADS1015_CONF_DR_250   0x01 ///< 250 SPS
#define ADS1015_CONF_DR_490   0x02 ///< 490 SPS
#define ADS1015_CONF_DR_920   0x03 ///< 920 SPS
#define ADS1015_CONF_DR_1600  0x04 ///< 1600 SPS
#define ADS1015_CONF_DR_2400  0x05 ///< 2400 SPS
#define ADS1015_CONF_DR_3300  0x06 ///< 3300 SPS

// Comparator mode
#define ADS1015_CONF_COMP_MODE_TRAD 0x00 ///< Traditional comparator
#define ADS1015_CONF_COMP_MODE_WDW  0x01 ///< Window comparator

// Comparator polarity
#define ADS1015_CONF_COMP_POL_ACTIVE_L 0x00 ///< Active low
#define ADS1015_CONF_COMP_POL_ACTIVE_H 0x01 ///< Active high

// Latching comparator
#define ADS1015_CONF_COMP_LAT_NONE  0x00 ///< Nonlatching comparator
#define ADS1015_CONF_COMP_LAT_LATCH 0x01 ///< Latching comparator

// Comparator queue and disable
#define ADS1015_CONF_COMP_QUEUE_1  0x00 ///< Assert after one conversion
#define ADS1015_CONF_COMP_QUEUE_2  0x01 ///< Assert after two conversions
#define ADS1015_CONF_COMP_QUEUE_4  0x02 ///< Assert after four conversions
#define ADS1015_CONF_COMP_QUEUE_0  0x03 ///< Disable comparator and set ALERT/RDY pin to high-impedance

typedef union {
	struct {
		uint16_t comp_queue:2;
		uint16_t comp_lat:1;
		uint16_t comp_pol:1;
		uint16_t comp_mode:1;
		uint16_t dr:3;
		uint16_t mode:1;
		uint16_t pga:3;
		uint16_t mux:3;
		uint16_t os:1;
	} config;

	struct {
		uint16_t val;
	} word;

	struct {
		#if (BYTE_ORDER == LITTLE_ENDIAN)
		uint8_t l;
		uint8_t h;
		#else
		uint8_t h;
		uint8_t l;
		#endif
	} byte;
} adc_ADS1015_reg_t;

driver_error_t *adc_ads1015_setup(adc_channel_t *chan);
driver_error_t *adc_ads1015_read(adc_channel_t *chan, int *raw, double *mvolts);

#endif /* _ADC_ADS1015_H */

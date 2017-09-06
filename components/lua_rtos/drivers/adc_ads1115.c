/*
 * Lua RTOS, ADC ADS1115 driver
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

#include "sdkconfig.h"

#if CONFIG_ADC_ADS1115

#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <drivers/adc.h>
#include <drivers/adc_ads1115.h>

/*
 * Helper functions
 */
driver_error_t *adc_ads1115_write_reg(uint8_t type, adc_ads1115_reg_t *reg, uint8_t address) {
	int transaction = I2C_TRANSACTION_INITIALIZER;
	uint8_t i2c = CONFIG_ADC_ADS1115_I2C;
	driver_error_t *error;
	uint8_t buff[3];

	buff[0] = type;
	buff[1] = reg->byte.h;
	buff[2] = reg->byte.l;

	error = i2c_start(i2c, &transaction);if (error) return error;
	error = i2c_write_address(i2c, &transaction, address, 0);if (error) return error;
	error = i2c_write(i2c, &transaction, (char *)&buff, sizeof(buff));if (error) return error;
	error = i2c_stop(i2c, &transaction);if (error) return error;

	return NULL;
}

driver_error_t *adc_ads1115_read_reg(uint8_t type, adc_ads1115_reg_t *reg, uint8_t address) {
	int transaction = I2C_TRANSACTION_INITIALIZER;
	uint8_t i2c = CONFIG_ADC_ADS1115_I2C;
	driver_error_t *error;
	uint8_t buff[1];
	uint16_t val;

	// Point to register
	buff[0] = type;

	error = i2c_start(i2c, &transaction);if (error) return error;
	error = i2c_write_address(i2c, &transaction, address, 0);if (error) return error;
	error = i2c_write(i2c, &transaction, (char *)&buff, sizeof(buff));if (error) return error;
	error = i2c_stop(i2c, &transaction);if (error) return error;

	// Read register
	error = i2c_start(i2c, &transaction);if (error) return error;
	error = i2c_write_address(i2c, &transaction, address, 1);if (error) return error;
	error = i2c_read(i2c, &transaction, (char *)&val, sizeof(val));if (error) return error;
	error = i2c_stop(i2c, &transaction);if (error) return error;

	#if (BYTE_ORDER == LITTLE_ENDIAN)
	reg->word.val = (uint16_t)(val >> 8) | ((uint16_t)(val & 0xff) << 8);
	#else
	reg->word.val = val;
	#endif

	return NULL;
}

/*
 * Operation functions
 */
driver_error_t *adc_ads1115_setup(adc_channel_t *chan) {
	driver_error_t *error;

	uint8_t i2c = CONFIG_ADC_ADS1115_I2C;
	int8_t channel = chan->channel;
	uint8_t address = chan->devid;

	if ((error = i2c_setup(i2c, I2C_MASTER, CONFIG_ADC_ADS1115_I2C_SPEED, 0, 0))) {
		return error;
	}

	if (!chan->setup) {
		syslog(
				LOG_INFO,
				"adc ADS1115 channel %d at i2c%d, address %x, vref+ %d, vref- %d, %d bits of resolution",
				channel, i2c, address, chan->pvref, chan->nvref, chan->resolution
		);
	}

	return NULL;
}

driver_error_t *adc_ads1115_read(adc_channel_t *chan, int *raw, double *mvolts) {
	driver_error_t *error;

	int8_t channel = chan->channel;
	uint8_t address = chan->devid;

	// Configure channel, and start a conversion
	adc_ads1115_reg_t reg;

	reg.word.val = 0;

	// Get PGA
	uint8_t pga = 0;
	if (chan->pvref <= 256) {
		pga = ADS1115_CONF_PGA_0256;
	} else if (chan->pvref <= 512) {
		pga = ADS1115_CONF_PGA_0512;
	} else if (chan->pvref <= 1024) {
		pga = ADS1115_CONF_PGA_1024;
	} else if (chan->pvref <= 2048) {
		pga = ADS1115_CONF_PGA_2048;
	} else if (chan->pvref <= 4096) {
		pga = ADS1115_CONF_PGA_4096;
	} else {
		pga = ADS1115_CONF_PGA_6144;
	}

	reg.config.mux = ADS1115_CONF_AIN0_GND + channel;
	reg.config.pga = pga;
	reg.config.dr = ADS1115_CONF_DR_128;
	reg.config.mode = ADS1115_CONF_MODE_SINGLE;
	reg.config.comp_queue = ADS1115_CONF_COMP_QUEUE_0;
	reg.config.os = ADS1115_CONF_START_CONV;

	error = adc_ads1115_write_reg(ADS1115_CONFIG, &reg, address);if (error) return error;

	// Wait for conversion
	for(;;) {
		error = adc_ads1115_read_reg(ADS1115_CONFIG, &reg, address);if (error) return error;
		if (reg.config.os == ADS1115_CONF_STATUS_IDLE) {
			break;
		}
	}

	// Read
	error = adc_ads1115_read_reg(ADS1115_AP_CONV, &reg, address);if (error) return error;

	*raw = reg.word.val;

	return NULL;
}

#endif

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
 * Lua RTOS, ADC ADS1015 driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_ADC_ADS1015

#include <string.h>

#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <drivers/adc.h>
#include <drivers/adc_ads1015.h>
#include <drivers/power_bus.h>

static int i2cdevice;

/*
 * Helper functions
 */
driver_error_t *adc_ads1015_write_reg(uint8_t type, adc_ADS1015_reg_t *reg, uint8_t address) {
	int transaction = I2C_TRANSACTION_INITIALIZER;
	driver_error_t *error;
	uint8_t buff[3];

	buff[0] = type;
	buff[1] = reg->byte.h;
	buff[2] = reg->byte.l;

	error = i2c_start(i2cdevice, &transaction);if (error) return error;
	error = i2c_write_address(i2cdevice, &transaction, address, 0);if (error) return error;
	error = i2c_write(i2cdevice, &transaction, (char *)&buff, sizeof(buff));if (error) return error;
	error = i2c_stop(i2cdevice, &transaction);if (error) return error;

	return NULL;
}

driver_error_t *adc_ads1015_read_reg(uint8_t type, adc_ADS1015_reg_t *reg, uint8_t address) {
	int transaction = I2C_TRANSACTION_INITIALIZER;
	driver_error_t *error;
	uint8_t buff[1];
	uint8_t val[2];

	// Point to register
	buff[0] = type;

	error = i2c_start(i2cdevice, &transaction);if (error) return error;
	error = i2c_write_address(i2cdevice, &transaction, address, 0);if (error) return error;
	error = i2c_write(i2cdevice, &transaction, (char *)&buff, sizeof(buff));if (error) return error;
	error = i2c_stop(i2cdevice, &transaction);if (error) return error;

	// Read register
	error = i2c_start(i2cdevice, &transaction);if (error) return error;
	error = i2c_write_address(i2cdevice, &transaction, address, 1);if (error) return error;
	error = i2c_read(i2cdevice, &transaction, (char *)&val, sizeof(val));if (error) return error;
	error = i2c_stop(i2cdevice, &transaction);if (error) return error;

	reg->byte.h = val[0];
	reg->byte.l = val[1];

	return NULL;
}

/*
 * Operation functions
 */
driver_error_t *adc_ads1015_setup(adc_chann_t *chan) {
	driver_error_t *error;

	uint8_t i2c = CONFIG_ADC_I2C;
	int8_t channel = chan->channel;
	uint8_t address = chan->devid;

	// Apply default max value
	if (chan->max == 0) {
		chan->max = 4096;
	}

	// Apply default address
	if (chan->devid == 0) {
		chan->devid = ADS1015_ADDR1;
	}

	// Apply default resolution if needed
	if (chan->resolution == 0) {
		chan->resolution = 12;
	}

	// Sanity checks
	if ((chan->max < 256) || (chan->max > 6144)) {
		return driver_error(ADC_DRIVER, ADC_ERR_INVALID_MAX, NULL);
	}

	if (chan->resolution != 12) {
		return driver_error(ADC_DRIVER, ADC_ERR_INVALID_RESOLUTION, NULL);
	}

#if CONFIG_LUA_RTOS_EXTERNAL_ADC_CONNECTED_TO_POWER_BUS
pwbus_on();
#endif

	// Attach
	if ((error = i2c_attach(i2c, I2C_MASTER, CONFIG_ADC_SPEED, 0, 0, &i2cdevice))) {
		return error;
	}

	if (chan->vref != 0) {
		return driver_error(ADC_DRIVER, ADC_ERR_VREF_SET_NOT_ALLOWED, NULL);
	}

	if (!chan->setup) {
		char pgas[16];

		if (chan->max <= 256) {
			chan->max = 256;
			strcpy(pgas,"+/- 256 mvolts");
		} else if (chan->max <= 512) {
			chan->max = 512;
			strcpy(pgas,"+/- 512 mvolts");
		} else if (chan->max <= 1024) {
			chan->max = 1024;
			strcpy(pgas,"+/- 1024 mvolts");
		} else if (chan->max <= 2048) {
			chan->max = 2048;
			strcpy(pgas,"+/- 2048 mvolts");
		} else if (chan->max <= 4096) {
			chan->max = 4096;
			strcpy(pgas,"+/- 4096 mvolts");
		} else {
			chan->max = 6144;
			strcpy(pgas,"+/- 6144 mvolts");
		}

		syslog(
				LOG_INFO,
				"adc ADS1015 channel %d at i2c%d, PGA %s, address %x, %d bits of resolution",
				channel, i2c, pgas, address, chan->resolution
		);
	}

	return NULL;
}

driver_error_t *adc_ads1015_read(adc_chann_t *chan, int *raw, double *mvolts) {
	driver_error_t *error;

	int8_t channel = chan->channel;
	uint8_t address = chan->devid;

	// Configure channel, and start a conversion
	adc_ADS1015_reg_t reg;

	reg.word.val = 0;

	// Get PGA
	uint8_t pga = 0;
	if (chan->max <= 256) {
		pga = ADS1015_CONF_PGA_0256;
	} else if (chan->max <= 512) {
		pga = ADS1015_CONF_PGA_0512;
	} else if (chan->max <= 1024) {
		pga = ADS1015_CONF_PGA_1024;
	} else if (chan->max <= 2048) {
		pga = ADS1015_CONF_PGA_2048;
	} else if (chan->max <= 4096) {
		pga = ADS1015_CONF_PGA_4096;
	} else {
		pga = ADS1015_CONF_PGA_6144;
	}

	reg.config.mux = ADS1015_CONF_AIN0_GND + channel;
	reg.config.pga = pga;
	reg.config.dr = ADS1015_CONF_DR_128;
	reg.config.mode = ADS1015_CONF_MODE_SINGLE;
	reg.config.comp_queue = ADS1015_CONF_COMP_QUEUE_0;
	reg.config.os = ADS1015_CONF_START_CONV;

	error = adc_ads1015_write_reg(ADS1015_CONF, &reg, address);if (error) return error;

	// Wait for conversion
	for(;;) {
		error = adc_ads1015_read_reg(ADS1015_CONF, &reg, address);if (error) return error;
		if (reg.config.os == ADS1015_CONF_STATUS_IDLE) {
			break;
		}
	}

	// Read
	error = adc_ads1015_read_reg(ADS1015_CONV, &reg, address);if (error) return error;

	reg.word.val = reg.word.val >> 3;

	if (raw) {
		*raw = reg.word.val;
	}

	if (mvolts) {
		*mvolts = (double)(double)((reg.word.val) * (chan->max)) / (double)chan->max_val;;
	}

	return NULL;
}

#endif

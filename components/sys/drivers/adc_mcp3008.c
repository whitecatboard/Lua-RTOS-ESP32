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
 * Lua RTOS, ADC MCP3008 driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_ADC_MCP3008

#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <drivers/adc.h>
#include <drivers/adc_mcp3008.h>
#include <drivers/power_bus.h>

static int spi_device = -1;

driver_error_t *adc_mcp3008_setup(adc_chann_t *chan) {
	driver_unit_lock_error_t *lock_error = NULL;
	driver_error_t *error;

	// Apply default resolution if needed
	if (chan->resolution == 0) {
		chan->resolution = 10;
	}

	if (chan->devid == 0) {
		chan->devid = CONFIG_ADC_MCP3008_CS;
	}

	if (chan->vref == 0) {
		chan->vref = CONFIG_LUA_RTOS_VDD;
	}

	// Sanity checks
	if (chan->max != 0) {
		return driver_error(ADC_DRIVER, ADC_ERR_MAX_SET_NOT_ALLOWED, NULL);
	}

	if (chan->resolution != 10) {
		return driver_error(ADC_DRIVER, ADC_ERR_INVALID_RESOLUTION, NULL);
	}

	chan->max = chan->vref;

	// Setup
#if CONFIG_LUA_RTOS_EXTERNAL_ADC_CONNECTED_TO_POWER_BUS
    pwbus_on();
#endif

    // Init SPI bus
	if (spi_device == -1) {
		if ((error = spi_setup(CONFIG_ADC_MCP3008_SPI, 1, chan->devid, 0, CONFIG_ADC_MCP3008_SPEED, SPI_FLAG_WRITE | SPI_FLAG_READ, &spi_device))) {
			return error;
		}
	}

	// Lock resources
	if ((error = spi_lock_bus_resources(CONFIG_ADC_MCP3008_SPI, DRIVER_ALL_FLAGS))) {
		return error;
	}

	if (!chan->setup) {
		syslog(
				LOG_INFO,
				"adc MCP3008 channel %d at spi%d, cs=%s%d, %d bits of resolution",
				chan->channel, CONFIG_ADC_MCP3008_SPI, gpio_portname(CONFIG_ADC_MCP3008_CS),
				gpio_name(CONFIG_ADC_MCP3008_CS),
				chan->resolution
		);
	}

    return NULL;
}

driver_error_t *adc_mcp3008_read(adc_chann_t *chan, int *raw, double *mvolts) {
	uint8_t buff[3];

	int channel = chan->channel;

	buff[0] =  ((0x18 | channel) & 0xf0) >> 4;
	buff[1] =  ((0x18 | channel) & 0x0f) << 4;

    spi_ll_select(spi_device);
    spi_ll_bulk_rw(spi_device, 3, buff);

    spi_ll_deselect(spi_device);

	int traw = (buff[1] & 0b11) << 8 | buff[2];
	double tmvolts = (double)(double)((traw) * (chan->max)) / (double)chan->max_val;

	if (raw) {
		*raw = traw;
	}

	if (mvolts) {
		*mvolts = tmvolts;
	}

    return NULL;
}

#endif

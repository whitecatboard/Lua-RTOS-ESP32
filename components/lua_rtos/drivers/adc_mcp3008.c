/*
 * Lua RTOS, ADC MCP3008 driver
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

#if CONFIG_ADC_MCP3008

#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <drivers/adc.h>
#include <drivers/adc_mcp3008.h>

static int spi_device = -1;

driver_error_t *adc_mcp3008_setup(adc_channel_t *chan) {
	driver_error_t *error;

    // Init SPI bus
	if (spi_device == -1) {
		if ((error = spi_setup(CONFIG_ADC_MCP3208_SPI, 1, chan->devid, 0, CONFIG_ADC_MCP3208_SPEED, SPI_FLAG_WRITE | SPI_FLAG_READ, &spi_device))) {
			return error;
		}
	}

	if (!chan->setup) {
		syslog(LOG_INFO, "adc MCP3208 channel %d at spi%d, cs=%s%d", chan->channel, CONFIG_ADC_MCP3208_SPI, gpio_portname(CONFIG_ADC_MCP3208_CS), gpio_name(CONFIG_ADC_MCP3208_CS));
	}

    return NULL;
}

driver_error_t *adc_mcp3008_read(adc_channel_t *chan, int *raw) {
	uint8_t buff[3];

	int channel = chan->channel;

	buff[0] =  ((0x18 | channel) & 0xf0) >> 4;
	buff[1] =  ((0x18 | channel) & 0x0f) << 4;

    spi_ll_select(spi_device);
    spi_ll_bulk_rw(spi_device, 3, buff);

    spi_ll_deselect(spi_device);

    *raw = (buff[1] & 0b11) << 8 | buff[2];

    return NULL;
}

#endif

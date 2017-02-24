/*
 * Lua RTOS, ADC MCP3008 driver
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

#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <drivers/adc.h>
#include <drivers/adc_mcp3008.h>

static uint8_t mcp3008_spi;
static uint8_t mcp3008_cs;

driver_error_t *adc_mcp3008_setup(int8_t unit, int8_t channel, uint8_t spi, uint8_t cs) {
	driver_error_t *error;

	mcp3008_spi = spi;
    mcp3008_cs = cs;

    // Init SPI bus
    if ((error = spi_init(mcp3008_spi, 1))) {
        return error;
    }

	syslog(LOG_INFO, "adc MCP3008 channel %d at spi%d, cs=%s%d", channel, spi, gpio_portname(cs), gpio_name(cs));

    return NULL;
}

driver_error_t *adc_mcp3008_read(int8_t unit, int8_t channel, int *raw) {
    uint8_t msb, lsb;

    spi_set_mode(mcp3008_spi, 0);
    spi_set_speed(mcp3008_spi, ADC_MCP3008_SPEED);
    spi_set_cspin(mcp3008_spi, mcp3008_cs);

    spi_select(mcp3008_spi);

    spi_transfer(mcp3008_spi, ((0x18 | channel) & 0xf0) >> 4, &msb);
    spi_transfer(mcp3008_spi, ((0x18 | channel) & 0x0f) << 4, &msb);
    spi_transfer(mcp3008_spi, 0, &lsb);

    spi_deselect(mcp3008_spi);

    *raw = ((msb & 0x03) << 8 | lsb);

    return NULL;
}

/*
 * Lua RTOS, ADC driver
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

#include "luartos.h"

#if USE_ADC
#include "freertos/FreeRTOS.h"

#include <stdint.h>

#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/cpu.h>
#include <drivers/adc.h>
#include <drivers/adc_internal.h>
#include "adc_mcp3008.h"
#include "adc_mcp3208.h"

static adc_unit_t adc_unit[CPU_LAST_ADC + 1];

// Driver message errors
DRIVER_REGISTER_ERROR(ADC, adc, InvalidUnit, "invalid unit", ADC_ERR_INVALID_UNIT);
DRIVER_REGISTER_ERROR(ADC, adc, InvalidChannel, "invalid channel", ADC_ERR_INVALID_CHANNEL);
DRIVER_REGISTER_ERROR(ADC, adc, InvalidResolution, "invalid resolution", ADC_ERR_INVALID_RESOLUTION);
DRIVER_REGISTER_ERROR(ADC, adc, NotEnoughtMemory, "not enough memory", ADC_ERR_NOT_ENOUGH_MEMORY);

/*
 * Helper functions
 */

/*
 * Operation functions
 */

// Get the ADC device number
driver_error_t *adc_device(int8_t unit, int8_t channel, uint8_t *device) {
	// Sanity checks
	if ((unit < CPU_FIRST_ADC) || (unit > CPU_LAST_ADC)) {
		return driver_operation_error(ADC_DRIVER, ADC_ERR_INVALID_UNIT, NULL);
	}

	if ((unit < CPU_FIRST_ADC_CH) || (unit > CPU_LAST_ADC_CH)) {
		return driver_operation_error(ADC_DRIVER, ADC_ERR_INVALID_CHANNEL, NULL);
	}

	if (unit == 1) {
		if (!((1 << channel) & CPU_ADC_ALL)) {
			return driver_operation_error(ADC_DRIVER, ADC_ERR_INVALID_CHANNEL, NULL);
		}

		*device = channel;
	} else {
		*device = ((CPU_LAST_ADC_CH + 1)) + channel;
	}

	return NULL;
}

// Setup ADC channel
driver_error_t *adc_setup(int8_t unit, int8_t channel, uint16_t vref, uint8_t resolution) {
	// Sanity checks
	if ((unit < CPU_FIRST_ADC) || (unit > CPU_LAST_ADC)) {
		return driver_operation_error(ADC_DRIVER, ADC_ERR_INVALID_UNIT, NULL);
	}

	if ((unit < CPU_FIRST_ADC_CH) || (unit > CPU_LAST_ADC_CH)) {
		return driver_operation_error(ADC_DRIVER, ADC_ERR_INVALID_CHANNEL, NULL);
	}

	// Allocate space for channels and locks, if needed
	if (!adc_unit[unit].channel) {
		adc_channel_t *channels;

		if (!(channels = calloc(CPU_LAST_ADC_CH + 1, sizeof(adc_channel_t)))) {
			return driver_operation_error(ADC_DRIVER, ADC_ERR_NOT_ENOUGH_MEMORY, NULL);
		}

		adc_unit[unit].channel = channels;
	}

	// Store channel configuration
	switch (unit) {
		case 1:
			adc_internal_setup(unit, channel);
			adc_unit[unit].channel[channel].max_resolution = 12;
			adc_unit[unit].channel[channel].vref = CPU_ADC_REF;
			break;

		case 2:
			adc_mcp3008_setup(unit, channel, 2, 15);
			adc_unit[unit].channel[channel].max_resolution = 10;
			adc_unit[unit].channel[channel].vref = vref;
			break;

		case 3:
			adc_mcp3208_setup(unit, channel, 2, 16);
			adc_unit[unit].channel[channel].max_resolution = 12;
			adc_unit[unit].channel[channel].vref = vref;
			break;
	}

	// Sanity checks
	if ((resolution < 6) || (resolution > adc_unit[unit].channel[channel].max_resolution)) {
		return driver_operation_error(ADC_DRIVER, ADC_ERR_INVALID_RESOLUTION, NULL);
	}

	adc_unit[unit].channel[channel].setup = 1;
	adc_unit[unit].channel[channel].resolution = resolution;

    switch (resolution) {
        case 6:  adc_unit[unit].channel[channel].max_val = 63;  break;
        case 7:  adc_unit[unit].channel[channel].max_val = 127; break;
        case 8:  adc_unit[unit].channel[channel].max_val = 255; break;
        case 9:  adc_unit[unit].channel[channel].max_val = 511; break;
        case 10: adc_unit[unit].channel[channel].max_val = 1023;break;
        case 11: adc_unit[unit].channel[channel].max_val = 2047;break;
        case 12: adc_unit[unit].channel[channel].max_val = 4095;break;
    }

	return NULL;
}

driver_error_t *adc_read(uint8_t unit, uint8_t channel, int *raw, double *mvols) {
	switch (unit) {
		case 1:
			adc_internal_read(unit, channel, raw);
			break;

		case 2:
			adc_mcp3008_read(unit, channel, raw);
			break;

		case 3:
			adc_mcp3208_read(unit, channel, raw);
			break;
	}

	// Normalize raw value to device resolution
	int resolution = adc_unit[unit].channel[channel].resolution;
	int max_val = adc_unit[unit].channel[channel].max_val;

	if (resolution != adc_unit[unit].channel[channel].max_resolution) {
		if (*raw & (1 << (adc_unit[unit].channel[channel].resolution - resolution - 1))) {
			*raw = ((*raw >> (adc_unit[unit].channel[channel].resolution - resolution)) + 1) & max_val;
		} else {
			*raw = *raw >> (adc_unit[unit].channel[channel].resolution - resolution);
		}
	}

	// Convert raw value to millivolts
	*mvols = ((double)(*raw) * (double)adc_unit[unit].channel[channel].vref) / (double)max_val;

	return NULL;
}

DRIVER_REGISTER(ADC,adc,NULL,NULL,NULL);

#endif

/*
-- mcp = adc.setup(adc.MCP3208, 0, 12, 3300)
--mcp:read()

mcp = adc.setup(adc.MCP3008, 0, 10, 3300)
mcp:read()

-- mcp = adc.setup(adc.ADC1, 7, 12)
while true do
	raw, mvolts = mcp:read()

    temp = (mvolts - 500) / 10
	print("raw: "..raw..", mvolts: "..mvolts..", temp: "..temp)
	tmr.delay(1)
end
*/

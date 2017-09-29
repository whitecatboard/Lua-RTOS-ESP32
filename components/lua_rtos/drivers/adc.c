/*
 * Lua RTOS, ADC driver
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

#include "freertos/FreeRTOS.h"

#include <stdint.h>

#include <sys/list.h>
#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/cpu.h>
#include <drivers/adc.h>
#include <drivers/adc_internal.h>
#include "adc_mcp3008.h"
#include "adc_mcp3208.h"
#include "adc_ads1115.h"

// Valid ADC devices
static const adc_dev_t adc_devs[] = {
	{"INTERNAL", adc_internal_setup, adc_internal_read},
#if CONFIG_ADC_MCP3008
	{"MCP3008", adc_mcp3008_setup, adc_mcp3008_read},
#else
	{NULL, NULL, NULL},
#endif
#if CONFIG_ADC_MCP3208
	{"MCP3208", adc_mcp3208_setup, adc_mcp3208_read},
#else
	{NULL, NULL, NULL},
#endif
#if CONFIG_ADC_ADS1115
	{"ADS1115", adc_ads1115_setup, adc_ads1115_read},
#else
	{NULL, NULL, NULL},
#endif
};

// List of channels
static struct list channels;

// Register driver and messages
static void _adc_init();

DRIVER_REGISTER_BEGIN(ADC,adc,NULL,_adc_init,NULL);
	DRIVER_REGISTER_ERROR(ADC, adc, InvalidUnit, "invalid unit", ADC_ERR_INVALID_UNIT);
	DRIVER_REGISTER_ERROR(ADC, adc, InvalidChannel, "invalid channel", ADC_ERR_INVALID_CHANNEL);
	DRIVER_REGISTER_ERROR(ADC, adc, InvalidResolution, "invalid resolution", ADC_ERR_INVALID_RESOLUTION);
	DRIVER_REGISTER_ERROR(ADC, adc, NotEnoughtMemory, "not enough memory", ADC_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(ADC, adc, InvalidPin, "invalid pin", ADC_ERR_INVALID_PIN);
	DRIVER_REGISTER_ERROR(ADC, adc, MaxSetupNotAllowed, "max value setup not allowed for this ADC", ADC_ERR_MAX_SET_NOT_ALLOWED);
	DRIVER_REGISTER_ERROR(ADC, adc, VrefSetupNotAllowed, "vref value setup not allowed for this ADC", ADC_ERR_VREF_SET_NOT_ALLOWED);
	DRIVER_REGISTER_ERROR(ADC, adc, InvalidMax, "invalid max value", ADC_ERR_INVALID_MAX);
	DRIVER_REGISTER_ERROR(ADC, adc, CannotCalibrate, "calibration is not allowed for this ADC", ADC_ERR_CANNOT_CALIBRATE);
	DRIVER_REGISTER_ERROR(ADC, adc, CalibrationError, "calibration error", ADC_ERR_CALIBRATION);
DRIVER_REGISTER_END(ADC,adc,NULL,_adc_init,NULL);

/*
 * Helper functions
 */

static void _adc_init() {
    list_init(&channels, 1);
}

static adc_channel_t *get_channel(int8_t unit, int8_t channel, int *item_index) {
	adc_channel_t *chan;
	int index;

    index = list_first(&channels);
    while (index >= 0) {
        list_get(&channels, index, (void **)&chan);

        if ((chan->unit == unit) && (chan->channel == channel)) {
        	*item_index = index;
        	return chan;
        }

        index = list_next(&channels, index);
    }

    return NULL;
}

/*
 * Operation functions
 */

// Setup ADC channel
driver_error_t *adc_setup(int8_t unit, int8_t channel, int16_t devid, int16_t vref, int16_t max, uint8_t resolution, adc_channel_h_t *h) {
	driver_error_t *error;

	// Sanity checks
	if (unit <= CPU_LAST_ADC) {
		// Internal ADC
		if ((unit < CPU_FIRST_ADC) || (unit > CPU_LAST_ADC)) {
			return driver_error(ADC_DRIVER, ADC_ERR_INVALID_UNIT, NULL);
		}

		if (unit == CPU_FIRST_ADC) {
			if (channel >= 32) {
				// Is a pin number
				if ((error = adc_internal_pin_to_channel(channel, (uint8_t *)&channel))) {
					return driver_error(ADC_DRIVER, ADC_ERR_INVALID_CHANNEL, NULL);
				}
			}

			if (!(CPU_ADC_ALL & (GPIO_BIT_MASK << channel))) {
				return driver_error(ADC_DRIVER, ADC_ERR_INVALID_CHANNEL, NULL);
			}
		}
	} else {
		if ((unit < CPU_FIRST_ADC) || (unit > CPU_LAST_ADC + 3)) {
			return driver_error(ADC_DRIVER, ADC_ERR_INVALID_UNIT, NULL);
		}

		if (!adc_devs[unit - CPU_FIRST_ADC].name) {
			return driver_error(ADC_DRIVER, ADC_ERR_INVALID_UNIT, adc_devs[unit].name);
		}
	}

	// Test if channel is setup
	int index = 0;
	adc_channel_t *chan = get_channel(unit, channel, &index);
	if (chan) {
		// Channel is setup, reuse the handle and reconfigure the adc
	}
	else {
		// Create space for the channel
		chan = calloc(1, sizeof(adc_channel_t));
		if (!chan) {
			return driver_error(ADC_DRIVER, ADC_ERR_NOT_ENOUGH_MEMORY, NULL);
		}
	}
	
	// Store channel configuration
	chan->unit = unit;
	chan->channel = channel;
	chan->devid = devid;
	chan->resolution = resolution;
	chan->vref = vref;
	chan->max = max;

	// Setup channel
	if ((error = adc_devs[unit - CPU_FIRST_ADC].setup(chan))) {
		free(chan);
		return error;
	}

	chan->max_val = ~(0xffff << chan->resolution);

	// At this point the channel is configured without errors
	
	if (!index) {
		// Store channel in channel list
		if (list_add(&channels, chan, &index)) {
			free(chan);
			return driver_error(ADC_DRIVER, ADC_ERR_NOT_ENOUGH_MEMORY, NULL);
		}
	}
	
	*h = (adc_channel_h_t)index;

	return NULL;
}

driver_error_t *adc_read(adc_channel_h_t *h, int *raw, double *mvolts) {
	driver_error_t *error = NULL;
	adc_channel_t *chan;

	// Get channel
	if (list_get(&channels, (int)*h, (void **)&chan)) {
		return driver_error(ADC_DRIVER, ADC_ERR_INVALID_CHANNEL, NULL);
	}

	if ((error = adc_devs[chan->unit - CPU_FIRST_ADC].read(chan, raw, mvolts))) {
		return error;
	}

	return NULL;
}

driver_error_t *adc_get_channel(adc_channel_h_t *h, adc_channel_t **chan) {
	adc_channel_t *channel;

    // Get channel
	if (list_get(&channels, (int)*h, (void **)&channel)) {
		return driver_error(ADC_DRIVER, ADC_ERR_INVALID_CHANNEL, NULL);
	}

	*chan = channel;

	return NULL;
}

driver_error_t *adc_read_avg(adc_channel_h_t *h, int samples, double *avgr, double *avgm) {
	driver_error_t *error;

	int raw;
	double mvolts = 0;
	int i;

	for(i=1; i <= samples;i++) {
		// Read value
		if ((error = adc_read(h, &raw, &mvolts))) {
			return error;
		}

		if (avgr) {
			*avgr = ((i - 1) * *avgr + raw) / i;
		}

		if (avgm) {
			*avgm = ((i - 1) * *avgm + mvolts) / i;
		}
	}

	return NULL;
}

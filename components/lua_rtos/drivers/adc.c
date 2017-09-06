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

static struct list channels;

// Driver message errors
DRIVER_REGISTER_ERROR(ADC, adc, InvalidUnit, "invalid unit", ADC_ERR_INVALID_UNIT);
DRIVER_REGISTER_ERROR(ADC, adc, InvalidChannel, "invalid channel", ADC_ERR_INVALID_CHANNEL);
DRIVER_REGISTER_ERROR(ADC, adc, InvalidResolution, "invalid resolution", ADC_ERR_INVALID_RESOLUTION);
DRIVER_REGISTER_ERROR(ADC, adc, NotEnoughtMemory, "not enough memory", ADC_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_ERROR(ADC, adc, InvalidPin, "invalid pin", ADC_ERR_INVALID_PIN);
DRIVER_REGISTER_ERROR(ADC, adc, InvalidVref, "invalid vref", ADC_ERR_INVALID_VREF);

/*
 * Helper functions
 */

static void _adc_init() {
    list_init(&channels, 1);
}

static adc_channel_t *get_channel(int8_t unit, int8_t channel) {
	adc_channel_t *chan;
	int index;

    index = list_first(&channels);
    while (index >= 0) {
        list_get(&channels, index, (void **)&chan);

        if ((chan->unit == unit) && (chan->channel == channel)) {
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
driver_error_t *adc_setup(int8_t unit, int8_t channel, int16_t devid, int16_t pvref, int16_t nvref, uint8_t resolution, adc_channel_h_t *h) {
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
		switch (unit) {
			case CPU_LAST_ADC + 1:
				#if !CONFIG_ADC_MCP3008
					return driver_error(ADC_DRIVER, ADC_ERR_INVALID_UNIT, "MCP3008 not enabled");
				#endif
				break;

			case CPU_LAST_ADC + 2:
				#if !CONFIG_ADC_MCP3208
					return driver_error(ADC_DRIVER, ADC_ERR_INVALID_UNIT, "MCP3208 not enabled");
				#endif
				break;

			case CPU_LAST_ADC + 3:
				#if !CONFIG_ADC_ADS1115
					return driver_error(ADC_DRIVER, ADC_ERR_INVALID_UNIT, "ADS1115 not enabled");
				#endif
				break;
		}
	}

	// Apply default vref+ / vref- if needed
	if (unit <= 1) {
		if (pvref == 0) {
			pvref = CONFIG_ADC_INTERNAL_VREF_P;
		}

		if (nvref == 0) {
			nvref = CONFIG_ADC_INTERNAL_VREF_N;
		}
	} else {
		if (pvref == 0) {
			pvref = CONFIG_ADC_EXTERNAL_VREF_P;
		}

		if (nvref == 0) {
			nvref = CONFIG_ADC_EXTERNAL_VREF_N;
		}
	}

	// Test if channel is setup
	adc_channel_t *chan;

	chan = get_channel(unit, channel);
	if (chan) {
		// Channel is setup, nothing to do
		return NULL;
	}

	// Create space for the channel
	chan = calloc(1, sizeof(adc_channel_t));
	if (!chan) {
		return driver_error(ADC_DRIVER, ADC_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Store channel configuration
	chan->unit = unit;
	chan->channel = channel;
	chan->devid = devid;
	chan->resolution = resolution;
	chan->pvref = pvref;
	chan->nvref = nvref;
	chan->rpvref = pvref;
	chan->rnvref = nvref;

	switch (unit) {
		case 1:
			// Sanity check on vref+ / vref-
			if (pvref > CONFIG_ADC_INTERNAL_VREF_P) {
				return driver_error(ADC_DRIVER, ADC_ERR_INVALID_VREF, "vref+");
			}

			if (nvref != 0) {
				return driver_error(ADC_DRIVER, ADC_ERR_INVALID_VREF, "vref-");
			}

			chan->nvref = nvref;

			if (pvref <= 1100) {
				chan->rpvref = 1100;
			} else if (pvref <= 1500) {
				chan->rpvref = 1500;
			} else if (pvref <= 2200) {
				chan->rpvref = 2200;
			} else {
				chan->rpvref = 3900;
			}

			chan->max_resolution = 12;

			break;

		case CPU_LAST_ADC + 1:
#if CONFIG_ADC_MCP3008
			chan->max_resolution = 10;

			if (devid == 0) {
				chan->devid = CONFIG_ADC_MCP3008_CS;
			}
#endif
			break;

		case CPU_LAST_ADC + 2:
#if CONFIG_ADC_MCP3208
			chan->max_resolution = 12;

			if (devid == 0) {
				chan->devid = CONFIG_ADC_MCP3208_CS;
			}
#endif
			break;
		case CPU_LAST_ADC + 3:
#if CONFIG_ADC_ADS1115
			if (pvref <= 256) {
				chan->rpvref = 256;
			} else if (pvref <= 512) {
				chan->rpvref = 512;
			} else if (pvref <= 1024) {
				chan->rpvref = 1024;
			} else if (pvref <= 2048) {
				chan->rpvref = 2048;
			} else if (pvref <= 4096) {
				chan->rpvref = 4096;
			} else {
				chan->rpvref = 6144;
			}

			if (nvref != 0) {
				return driver_error(ADC_DRIVER, ADC_ERR_INVALID_VREF, "vref-");
			}

			// Apply default adress
			if (devid == 0) {
				chan->devid = ADS1115_ADDR1;
			}

			chan->max_resolution = 15;
#endif
			break;
	}

	// Apply default resolution if needed
	if (chan->resolution == 0) {
		chan->resolution = chan->max_resolution;
	}

	// Sanity checks on resolution
	if ((chan->resolution < 6) || (chan->resolution > chan->max_resolution)) {
		return driver_error(ADC_DRIVER, ADC_ERR_INVALID_RESOLUTION, NULL);
	}

    switch (chan->resolution) {
        case 6:  chan->max_val = 63;  break;
        case 7:  chan->max_val = 127; break;
        case 8:  chan->max_val = 255; break;
        case 9:  chan->max_val = 511; break;
        case 10: chan->max_val = 1023;break;
        case 11: chan->max_val = 2047;break;
        case 12: chan->max_val = 4095;break;
        case 13: chan->max_val = 8193;break;
        case 14: chan->max_val = 16383;break;
        case 15: chan->max_val = 32767;break;
        case 16: chan->max_val = 65535;break;
    }

    // Setup the channel
	switch (unit) {
		case 1:
			 error = adc_internal_setup(chan);
			break;

		case 2:
#if CONFIG_ADC_MCP3008
			error = adc_mcp3008_setup(chan);
#endif
			break;

		case 3:
#if CONFIG_ADC_MCP3208
			error = adc_mcp3208_setup(chan);
#endif
			break;
		case 4:
#if CONFIG_ADC_ADS1115
			error = adc_ads1115_setup(chan);
#endif
 			break;
	 }

	 if (error) {
		 free(chan);
		 return error;
	 }

	// At this point the channel is configured without errors
	// Store channel in channel list
	int index;

    if (list_add(&channels, chan, &index)) {
    	free(chan);
		return driver_error(ADC_DRIVER, ADC_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    *h = (adc_channel_h_t)index;

	return NULL;
}

driver_error_t *adc_read(adc_channel_h_t *h, int *araw, double *amvolts) {
	driver_error_t *error = NULL;
	adc_channel_t *chan;

	int raw;
	double mvolts;

    // Get channel
	if (list_get(&channels, (int)*h, (void **)&chan)) {
		return driver_error(ADC_DRIVER, ADC_ERR_INVALID_CHANNEL, NULL);
	}

	switch (chan->unit) {
		case 1:
			error = adc_internal_read(chan, &raw, &mvolts);
			break;

		case CPU_LAST_ADC + 1:
#if CONFIG_ADC_MCP3008
			error = adc_mcp3008_read(chan, &raw, &mvolts);
#endif
			break;

		case CPU_LAST_ADC + 2:
#if CONFIG_ADC_MCP3208
			error = adc_mcp3208_read(chan, &raw, &mvolts);
#endif
			break;

		case CPU_LAST_ADC + 3:
#if CONFIG_ADC_ADS1115
			error = adc_ads1115_read(chan, &raw, &mvolts);
#endif
			break;
	}

	if (error) {
		return error;
	}

	// Normalize raw value to device resolution
	int resolution = chan->resolution;
	int max_val = chan->max_val;

	if (resolution != chan->max_resolution) {
		if (raw & (1 << (chan->max_resolution - resolution - 1))) {
			raw = ((raw >> (chan->max_resolution - resolution)) + 1) & max_val;
		} else {
			raw = raw >> (chan->max_resolution - resolution);
		}
	}

	if (chan->unit != 1) {
		// Convert raw value to mVolts
		mvolts = (double)chan->rnvref +  (double)((raw) * (chan->rpvref - chan->rnvref)) / (double)max_val;
	}

	if (araw) {
		*araw = raw;
	}

	if (amvolts) {
		*amvolts = mvolts;
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

DRIVER_REGISTER(ADC,adc,NULL,_adc_init,NULL);

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
 * Lua RTOS, ADC internal driver
 *
 */

#include "sdkconfig.h"

#include "esp_adc_cal.h"
#include "driver/adc.h"

#include <stdint.h>
#include <string.h>

#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <drivers/adc_internal.h>

/*
 * Helper functions
 */

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
// Get the pins used by an ADC channel
static void adc_pins(int8_t channel, uint8_t *pin) {
	switch (channel) {
		case 0: *pin = GPIO36; break;
        case 1: *pin = GPIO37; break;
        case 2: *pin = GPIO38; break;
		case 3: *pin = GPIO39; break;
		case 4: *pin = GPIO32; break;
		case 5: *pin = GPIO33; break;
		case 6: *pin = GPIO34; break;
		case 7: *pin = GPIO35; break;
	}
}

// Lock resources needed by ADC
static driver_error_t *adc_lock_resources(int8_t channel, void *resources) {
	adc_resources_t tmp_adc_resources;

	if (!resources) {
		resources = &tmp_adc_resources;
	}

	adc_resources_t *adc_resources = (adc_resources_t *)resources;
    driver_unit_lock_error_t *lock_error = NULL;

    adc_pins(channel, &adc_resources->pin);

    // Lock this pins
    if ((lock_error = driver_lock(ADC_DRIVER, channel, GPIO_DRIVER, adc_resources->pin, DRIVER_ALL_FLAGS, NULL))) {
    	// Revoked lock on pin
    	return driver_lock_error(ADC_DRIVER, lock_error);
    }

    return NULL;
}
#endif

/*
 * Operation functions
 *
 */

driver_error_t * adc_internal_pin_to_channel(uint8_t pin, uint8_t *chan) {
	switch (pin) {
		case GPIO36: *chan = 0; break;
        case GPIO37: *chan = 1; break;
        case GPIO38: *chan = 2; break;
		case GPIO39: *chan = 3; break;
		case GPIO32: *chan = 4; break;
		case GPIO33: *chan = 5; break;
		case GPIO34: *chan = 6; break;
		case GPIO35: *chan = 7; break;
		default:
			return driver_error(ADC_DRIVER, ADC_ERR_INVALID_PIN, NULL);
	}

	return NULL;
}

driver_error_t *adc_internal_setup(adc_chann_t *chan) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	driver_error_t *error;
#endif

	adc_resources_t resources = {0};
	adc_atten_t atten;
	char attens[6];

	uint8_t unit = chan->unit;
	uint8_t channel = chan->channel;

	// Apply default max value
	if (chan->max == 0) {
		chan->max = 3900;
	}

	// Apply default resolution if needed
	if (chan->resolution == 0) {
		chan->resolution = 12;
	}

	// Sanity checks
	if ((chan->max < 0) || (chan->max > 3900)) {
		return driver_error(ADC_DRIVER, ADC_ERR_INVALID_MAX, NULL);
	}

	if ((chan->resolution != 9) && (chan->resolution != 10) && (chan->resolution != 11) && (chan->resolution != 12)) {
		return driver_error(ADC_DRIVER, ADC_ERR_INVALID_RESOLUTION, NULL);
	}

	if (chan->vref != 0) {
		return driver_error(ADC_DRIVER, ADC_ERR_VREF_SET_NOT_ALLOWED, NULL);
	}

	// Setup

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	// Lock the resources needed
	if ((error = adc_lock_resources(channel, &resources))) {
		return error;
	}
#endif

	// Computes the required attenuation
	if (chan->max <= 1100) {
		atten = ADC_ATTEN_0db;
		strcpy(attens, "0db");
	} else if (chan->max <= 1500) {
		atten = ADC_ATTEN_2_5db;
		strcpy(attens, "2.5db");
	} else if (chan->max <= 2200) {
		atten = ADC_ATTEN_6db;
		strcpy(attens, "6db");
	} else {
		atten = ADC_ATTEN_11db;
		strcpy(attens, "11db");
	}

	adc1_config_channel_atten(channel, atten);

	// Configure all channels with the given resolution
	adc1_config_width(chan->resolution - 9);

	// Get characteristics
	if (!chan->chars) {
	    chan->chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	    if (!chan->chars) {
	        return driver_error(ADC_DRIVER, ADC_ERR_NOT_ENOUGH_MEMORY, NULL);
	    }
	}

	esp_adc_cal_characterize(1, atten, chan->resolution - 9, CONFIG_ADC_INTERNAL_VREF, chan->chars);

	if (!chan->setup) {
		syslog(
				LOG_INFO,
				"adc%d: at pin %s%d, attenuation %s, %d bits of resolution", unit, gpio_portname(resources.pin),
				gpio_name(resources.pin), attens, chan->resolution
		);
	}

	return NULL;
}

driver_error_t *adc_internal_read(adc_chann_t *chan, int *raw, double *mvolts) {
	uint8_t channel = chan->channel;

	int traw = adc1_get_raw(channel);
	double tmvolts = esp_adc_cal_raw_to_voltage(traw, chan->chars);

	if (raw) {
		*raw = traw;
	}

	if (mvolts) {
		*mvolts = tmvolts;
	}

	return NULL;
}

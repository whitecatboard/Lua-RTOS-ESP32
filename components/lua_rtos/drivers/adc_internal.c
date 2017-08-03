/*
 * Lua RTOS, ADC internal driver
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

#include "luartos.h"

#include "driver/adc.h"

#include <stdint.h>

#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <drivers/adc_internal.h>

/*
 * Helper functions
 */

// Get the pins used by an ADC channel
static void adc_pins(int8_t channel, uint8_t *pin) {
	switch (channel) {
		case 0: *pin = GPIO36; break;
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

/*
 * Operation functions
 *
 */

driver_error_t * adc_internal_pin_to_channel(uint8_t pin, uint8_t *chan) {
	switch (pin) {
		case GPIO36: *chan = 0; break;
		case GPIO39: *chan = 3; break;
		case GPIO32: *chan = 4; break;
		case GPIO33: *chan = 5; break;
		case GPIO34: *chan = 6; break;
		case GPIO35: *chan = 7; break;
		default:
			return driver_operation_error(ADC_DRIVER, ADC_ERR_INVALID_PIN, NULL);
	}

	return NULL;
}

driver_error_t *adc_internal_setup(adc_channel_t *chan) {
	driver_error_t *error;
	adc_resources_t resources = {0};
	adc_atten_t atten;

	uint8_t unit = chan->unit;
	uint8_t channel = chan->channel;

	// Lock the resources needed
	if ((error = adc_lock_resources(channel, &resources))) {
		return error;
	}

	// Computes the required attenuation
	if (chan->pvref <= 1100) {
		atten = ADC_ATTEN_0db;
	} else if (chan->pvref <= 1500) {
		atten = ADC_ATTEN_2_5db;
	} else if (chan->pvref <= 2200) {
		atten = ADC_ATTEN_6db;
	} else {
		atten = ADC_ATTEN_11db;
	}

	adc1_config_channel_atten(channel, atten);

	// Configure all channels with a 12-bit resolution
	adc1_config_width(ADC_WIDTH_12Bit);

	if (!chan->setup) {
		syslog(LOG_INFO, "adc%d: at pin %s%d", unit, gpio_portname(resources.pin), gpio_name(resources.pin));
	}

	return NULL;
}

driver_error_t *adc_internal_read(adc_channel_t *chan, int *raw) {
	uint8_t channel = chan->channel;

	*raw = adc1_get_voltage(channel);

	return NULL;
}

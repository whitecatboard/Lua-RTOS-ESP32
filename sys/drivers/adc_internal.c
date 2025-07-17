/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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

#include "driver.h"
#include "syslog.h"

#include "gpio.h"
#include "adc.h"
#include "adc_internal.h"

#include "esp_adc/adc_oneshot.h"

#include <stdint.h>
#include <string.h>

typedef struct {
	adc_oneshot_unit_handle_t hndl;
	adc_cali_handle_t cal_hndl;
} adc_internal_t;

static adc_internal_t _adc_internal[1] = {0};

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

static adc_cali_handle_t adc_calibration_init(adc_channel_t channel, adc_atten_t atten) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    return handle;
}

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
		atten = ADC_ATTEN_DB_0;
		strcpy(attens, "0db");
	} else if (chan->max <= 1500) {
		atten = ADC_ATTEN_DB_2_5;
		strcpy(attens, "2.5db");
	} else if (chan->max <= 2200) {
		atten = ADC_ATTEN_DB_6;
		strcpy(attens, "6db");
	} else {
		atten = ADC_ATTEN_DB_12;
		strcpy(attens, "11db");
	}

	if (_adc_internal[unit].hndl == NULL) {
		// Init ADC
		adc_oneshot_unit_init_cfg_t init_config = {
			.unit_id = ADC_UNIT_1,
		};

		adc_oneshot_new_unit(&init_config, &_adc_internal[unit].hndl);
	}

	// Configure ADC
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = atten,
    };

    adc_oneshot_config_channel(_adc_internal[unit].hndl, channel, &config);

    // Init calibration
    _adc_internal[unit].cal_hndl = adc_calibration_init(channel, atten);

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
	int traw = 0;
	int tvoltsi = 0;

	adc_oneshot_read(_adc_internal[chan->unit].hndl, chan->channel, &traw);

	if (_adc_internal[chan->unit].cal_hndl != NULL) {
		adc_cali_raw_to_voltage(_adc_internal[chan->unit].cal_hndl, traw, &tvoltsi);
	}

	if (raw) {
		*raw = traw;
	}

	if (mvolts) {
		*mvolts = (double)tvoltsi;
	}

	return NULL;
}

/*
 * Lua RTOS, ADC driver
 *
 * Copyright (C) 2015 - 2016
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

#include "soc/soc.h"
#include "soc/rtc_io_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include "driver/adc.h"

#include <stdint.h>

#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/cpu.h>
#include <drivers/adc.h>

// This macro gets a reference for this driver into drivers array
#define ADC_DRIVER driver_get_by_name("adc")

// Driver locks
driver_unit_lock_t adc_locks[CPU_LAST_ADC + 1];

// Driver message errors
DRIVER_REGISTER_ERROR(ADC, adc, CannotSetup, "can't setup", ADC_ERR_CANT_INIT);
DRIVER_REGISTER_ERROR(ADC, adc, InvalidChannel, "invalid channel", ADC_ERR_INVALID_CHANNEL);

/*s
 * Operation functions
 *
 */

// Get the pins used by an ADC channel
void adc_pins(int8_t channel, uint8_t *pin) {
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
driver_error_t *adc_lock_resources(int8_t channel, void *resources) {
	adc_resources_t tmp_adc_resources;

	if (!resources) {
		resources = &tmp_adc_resources;
	}

	adc_resources_t *adc_resources = (adc_resources_t *)resources;
    driver_unit_lock_error_t *lock_error = NULL;

    adc_pins(channel, &adc_resources->pin);

    // Lock this pins
    if ((lock_error = driver_lock(ADC_DRIVER, channel, GPIO_DRIVER, adc_resources->pin))) {
    	// Revoked lock on pin
    	return driver_lock_error(ADC_DRIVER, lock_error);
    }

    return NULL;
}

// ADC setup
driver_error_t *adc_setup(void) {
	adc1_config_width(ADC_WIDTH_12Bit);

	return NULL;
}

// Setup an ADC channel
driver_error_t *adc_setup_channel(int8_t channel) {
	// Sanity checks
	if (!((1 << channel) & CPU_ADC_ALL)) {
		return driver_operation_error(ADC_DRIVER, ADC_ERR_INVALID_CHANNEL, NULL);
	}

    // Lock resources
    driver_error_t *error;
    adc_resources_t resources;

    if ((error = adc_lock_resources(channel, &resources))) {
		return error;
	}

    adc1_config_channel_atten(channel, ADC_ATTEN_0db);

    syslog(LOG_INFO, "adc%d: at pin %s%d", channel, gpio_portname(resources.pin), gpio_name(resources.pin));

    return NULL;
}

driver_error_t *adc_read(int8_t channel, int *val) {
	// Sanity checks
	if (!((1 << channel) & CPU_ADC_ALL)) {
		return driver_operation_error(ADC_DRIVER, ADC_ERR_INVALID_CHANNEL, NULL);
	}

	*val = adc1_get_voltage(channel);

	return NULL;
}

DRIVER_REGISTER(ADC,adc,adc_locks,NULL,NULL);

#endif

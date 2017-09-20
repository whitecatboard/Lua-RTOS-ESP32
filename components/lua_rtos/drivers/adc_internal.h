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

#ifndef _ADC_INTERNAL_H
#define	_ADC_INTERNAL_H

#include <drivers/adc.h>
#include <stdint.h>

typedef struct {
	driver_unit_lock_t *lock;
} adc_lock_t;

typedef struct {
	uint8_t pin;
} adc_resources_t;

driver_error_t * adc_internal_pin_to_channel(uint8_t pin, uint8_t *chan);
driver_error_t *adc_internal_setup(adc_channel_t *chan);
driver_error_t *adc_internal_read(adc_channel_t *chan, int *raw, double *mvolts);

#endif	/* _ADC_INTERNAL_H */

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

#ifndef ADC_H
#define	ADC_H

#include <stdint.h>

#include "driver/adc.h"

#include <drivers/cpu.h>
#include <sys/driver.h>

// Channel handler
typedef uint32_t *adc_channel_h_t;

// ADC channel
typedef struct {
	uint8_t unit;            ///< ADC unit
	uint8_t channel;         ///< Channel number
	int devid;	             ///< Device id
	uint8_t setup;           ///< Channel is setup?
	uint8_t max_resolution;  ///< Max resolution supported for channel
	uint8_t resolution;      ///< Current resolution
	uint16_t max_val;        ///< Max value, depends on resolution
	int16_t pvref;           ///< Positive voltage reference in mvolts
	int16_t nvref;           ///< Negative voltage reference in mvolts
} adc_channel_t;

// ADC errors
#define ADC_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  0)
#define ADC_ERR_INVALID_CHANNEL          (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  1)
#define ADC_ERR_INVALID_RESOLUTION       (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  2)
#define ADC_ERR_NOT_ENOUGH_MEMORY	 	 (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  3)
#define ADC_ERR_INVALID_PIN				 (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  4)
#define ADC_ERR_INVALID_VREF		     (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  5)

/**
 * @brief Setup an adc channel.
 *
 * @param unit ADC unit identifier.
 * @param channel ADC channel number of the ADC unit.
 * @param devid Device id. This is used for SIP & I2C external ADC devices for identify the device in
 *              the bus. For example in SPI devid contains the CS GPIO, and in I2C contains the device
 *              address.
 * @param pvref Positive voltage reference in mVolts.
 * @param nvref Negative voltage reference in mVolts.
 * @param resolution Bits of resolution.
 * @param h A pointer to the channel handler. This handler is used later for refer to the channel.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs. Error can be an operation error or a lock error.
 */
driver_error_t *adc_setup(int8_t unit, int8_t channel, int16_t devid, int16_t pvref, int16_t nvref, uint8_t resolution, adc_channel_h_t *h);

/**
 * @brief Read from an adc channel.
 *
 * @param h A pointer to a channel handler.
 * @param raw A pointer to an int variable that holds the raw value from the ADC.
 * @param mvolts A pointer to a double variable that holds the raw value from the ADC converted to mVolts.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs. Error can be an operation error or a lock error.
 */
driver_error_t *adc_read(adc_channel_h_t *h, int *raw, double *mvols);

driver_error_t *adc_get_channel(adc_channel_h_t *h, adc_channel_t **chan);

#endif	/* ADC_H */

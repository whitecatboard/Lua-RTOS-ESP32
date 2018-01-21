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
 * Lua RTOS, ADC driver
 *
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
	uint8_t resolution;      ///< Current resolution
	uint16_t max_val;        ///< Max value, depends on resolution
	int16_t vref;             ///< VREF voltage attached in mvolts
	int16_t max;             ///< Max voltage attached in mvolts
} adc_chann_t;

// Adc devices
typedef struct {
	const char *name;
	driver_error_t *(*setup)(adc_chann_t *);
	driver_error_t *(*read)(adc_chann_t *, int *, double *);
} adc_dev_t;

// ADC errors
#define ADC_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  0)
#define ADC_ERR_INVALID_CHANNEL          (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  1)
#define ADC_ERR_INVALID_RESOLUTION       (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  2)
#define ADC_ERR_NOT_ENOUGH_MEMORY	 	 (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  3)
#define ADC_ERR_INVALID_PIN				 (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  4)
#define ADC_ERR_MAX_SET_NOT_ALLOWED		 (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  5)
#define ADC_ERR_VREF_SET_NOT_ALLOWED     (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  6)
#define ADC_ERR_INVALID_MAX				 (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  7)
#define ADC_ERR_CANNOT_CALIBRATE	     (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  8)
#define ADC_ERR_CALIBRATION	             (DRIVER_EXCEPTION_BASE(ADC_DRIVER_ID) |  9)

extern const int adc_errors;
extern const int adc_error_map;

/**
 * @brief Setup an adc channel.
 *
 * @param unit ADC unit identifier.
 * @param channel ADC channel number of the ADC unit.
 * @param devid Device id. This is used for SIP & I2C external ADC devices for identify the device in
 *              the bus. For example in SPI devid contains the CS GPIO, and in I2C contains the device
 *              address.
 * @param vref Voltage reference in mVolts. 0 = default.
 * @param max Max voltage attached to ADC channel in mVolts. 0 = default.
 * @param resolution Bits of resolution.
 * @param h A pointer to the channel handler. This handler is used later for refer to the channel.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs. Error can be an operation error or a lock error.
 */
driver_error_t *adc_setup(int8_t unit, int8_t channel, int16_t devid, int16_t vref, int16_t max, uint8_t resolution, adc_channel_h_t *h);

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

/**
 * @brief Read from an adc channel taking some samples and doing the average.
 *
 * @param h A pointer to a channel handler.
 * @param samples Number of samples.
 * @param raw A pointer to a double variable that holds the average raw value from the ADC.
 * @param mvolts A pointer to a double variable that holds the average raw value from the ADC converted to mVolts.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs. Error can be an operation error or a lock error.
 */
driver_error_t *adc_read_avg(adc_channel_h_t *h, int samples, double *avgr, double *avgm);

/**
 * @brief Get the ADC channel from a channel handler.
 *
 * @param h A pointer to a channel handler.
 * @param chan A pointer to the channel.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs. Error can be an operation error or a lock error.
 */
driver_error_t *adc_get_channel(adc_channel_h_t *h, adc_chann_t **chan);

#endif	/* ADC_H */

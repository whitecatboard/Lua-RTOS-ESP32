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

#if 0

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_DAC

#include <math.h>

#include <driver/gpio.h>
#include <driver/dac.h>

#include "soc/rtc_io_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"

#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/dac.h>

DRIVER_REGISTER_BEGIN(DAC,dac,2,NULL,NULL);
	DRIVER_REGISTER_ERROR(DAC, dac, InvalidPin, "invalid pin", DAC_ERR_INVALID_PIN);
DRIVER_REGISTER_END(DAC,dac,2,NULL,NULL);

static int pin_to_chan(int pin) {
	if (pin == 25) {
		return DAC_CHANNEL_1;
	}

	if (pin == 26) {
		return DAC_CHANNEL_2;
	}

	return -1;
}

void dac_scale_set(dac_channel_t channel, int scale)
{
    switch(channel) {
        case DAC_CHANNEL_1:
            SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE1, scale, SENS_DAC_SCALE1_S);
            break;
        case DAC_CHANNEL_2:
            SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE2, scale, SENS_DAC_SCALE2_S);
            break;
        default :
           printf("Channel %d\n", channel);
    }
}

driver_error_t *dac_tone_setup(int pin) {
	// Sanity checks
	int channel;

	// Get DAC channel
	channel = pin_to_chan(pin);
	if (channel < 0) {
		return driver_error(DAC_DRIVER, DAC_ERR_INVALID_PIN, NULL);
	}

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    // Lock resources
    driver_unit_lock_error_t *lock_error = NULL;

    if ((lock_error = driver_lock(DAC_DRIVER, channel, GPIO_DRIVER, pin, DRIVER_ALL_FLAGS, NULL))) {
        return driver_lock_error(DAC_DRIVER, lock_error);
    }
#endif

    // Enable tone generator common to both channels
    SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
    switch(channel) {
        case DAC_CHANNEL_1:
            // Enable / connect tone tone generator on / to this channel
            SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);

            // Invert MSB, otherwise part of waveform will have inverted
            SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, 2, SENS_DAC_INV1_S);
            break;
        case DAC_CHANNEL_2:
            SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
            SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV2, 2, SENS_DAC_INV2_S);
            break;
    }

    // Enable DAC output
    dac_output_enable(channel);

    //dac_scale_set(channel, 2);

	return NULL;
}



void dac_tone_play(float frequency, uint32_t duration) {
	// Base frequency is 8 MHz
	uint32_t clk = 3;

	// Compute the steps
	uint32_t steps = round(((frequency * (1.0 + (float)clk)) * 65536.0) / (float)(RTC_FAST_CLK_FREQ_APPROX));

	//uint32_t steps = round((frequency * 65536.0) / (float)1000000);

	// Set frequency
    REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, clk);
    SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, steps, SENS_SW_FSTEP_S);

    vTaskDelay(duration / portTICK_PERIOD_MS);
}

#endif

#endif

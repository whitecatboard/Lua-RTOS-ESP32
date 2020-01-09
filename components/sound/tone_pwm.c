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
 * Lua RTOS, tone PWM library
 *
 */

#include "sound.h"

#include <drivers/pwm.h>

#include <sys/delay.h>

/*
 * Operation functions
 */
driver_error_t *tone_pwm_setup(tone_pwm_config_t *config, tone_pwm_device_h_t *h) {
	// Allocate space for device handle
	*h = calloc(1, sizeof(tone_pwm_device_t));

	if (!*h) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// To generate sound we will use a square PWM signal with 50% duty
	driver_error_t *error = pwm_setup(config->unit, -1, config->pin, 440, 0.5, &(*h)->channel);
	if (error) {
		tone_pwm_unsetup(h);
		return error;
	}

	(*h)->unit = config->unit;

	return NULL;
}

void tone_pwm_unsetup(tone_pwm_device_h_t *h) {
	if (!*h) return;

	pwm_unsetup((*h)->unit, (*h)->channel);
	free(*h);
	*h= NULL;
}

driver_error_t *tone_pwm_play(tone_pwm_device_h_t *h, uint32_t freq, uint32_t duration) {
	if (!*h) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_NO_TONE_GEN, NULL);
	}

	// Set frequency, duty is set at 50% in tone_setup call
	driver_error_t *error = pwm_set_freq((*h)->unit, (*h)->channel, freq);
	if (error) {
		return error;
	}

	// Start PWM generation
	error = pwm_start((*h)->unit, (*h)->channel);
	if (error) {
		return error;
	}

	// Wait for duration
	delay(duration);

	// Stop PWM generation
	error = pwm_stop((*h)->unit, (*h)->channel);
	if (error) {
		return error;
	}

	return NULL;
}


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
 * Lua RTOS, tone library
 *
 */

#include "sound.h"

#include <sys/driver.h>

driver_error_t *tone_setup(tone_gen_t gen, tone_gen_config_t *config, tone_gen_device_h_t *h) {
	driver_error_t *error;

	// Sanity checks
	if ((gen < ToneGeneratorPWM) || (gen >= ToneGeneratorMAX)) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_INVALID_TONE_GEN, NULL);
	}

	// Allocate space for device handle
	*h = calloc(1, sizeof(tone_gen_device_t));
	if (!*h) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Setup tone generator
	switch (gen) {
		case ToneGeneratorPWM:
			(*h)->_unsetup = (tone_unsetup_t)tone_pwm_unsetup;
			(*h)->_play = (tone_play_t)tone_pwm_play;
			(*h)->_set_volume = NULL;

			error = tone_pwm_setup(&config->pwm, (tone_pwm_device_h_t *)(&(*h)->h));
			if (error) {
				tone_unsetup(h);
			}

			break;

		case ToneGeneratorDAC:
			(*h)->_unsetup = (tone_unsetup_t)tone_dac_unsetup;
			(*h)->_play = (tone_play_t)tone_dac_play;
			(*h)->_set_volume = (set_volume_t)tone_dac_set_volume;

			error = tone_dac_setup(&config->dac, (tone_dac_device_h_t *)(&(*h)->h));
			if (error) {
				tone_unsetup(h);
			}

			break;

		case ToneGeneratorMAX:
			break;
	}

	// Set default volume
	tone_set_volume(h, 1.0);

	return NULL;
}

driver_error_t *tone_unsetup(tone_gen_device_h_t *h) {
	if (!*h) return NULL;

	(*h)->_unsetup((void **)(&((*h)->h)));
	free(*h);
	*h = NULL;

	return NULL;
}

driver_error_t *tone_set_volume(tone_gen_device_h_t *h, float volume) {
	if (!*h) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_NO_TONE_GEN, NULL);
	}

	if ((volume < 0) || (volume > 1)) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_NO_INVALID_VOLUME, NULL);
	}

	if ((*h)->_set_volume) {
		(*h)->_set_volume((void **)(&((*h)->h)), volume);
	}

	return NULL;
}

driver_error_t *tone_play(tone_gen_device_h_t *h, uint32_t freq, uint32_t duration) {
	if (!*h) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_NO_TONE_GEN, NULL);
	}

	driver_error_t *error = (*h)->_play((void **)(&(*h)->h), freq, duration);
	if (error) {
		return error;
	}

	return NULL;
}

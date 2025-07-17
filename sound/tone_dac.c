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
 * Lua RTOS, tone DAC library
 *
 */

#include "sound.h"
#include "driver/dac_continuous.h"

#include <math.h>

#define TONE_DAC_SAMPLE_RATE 38000

typedef struct {
	dac_continuous_handle_t hndl;
	tone_dac_device_h_t device;
} tone_dac_t;

static tone_dac_t _dac[2] = {0};

/*
 * Helper functions
 */
static double map(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*
 * Operation functions
 */
driver_error_t *tone_dac_setup(tone_dac_config_t *config, tone_dac_device_h_t *h) {
	// Sanity checks
	if ((config->pin != 25) && (config->pin != 26)) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_INVALID_PIN, NULL);
	}

	if (!h) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_INVALID_DAC_DEVICE, NULL);
	}

	// Get DAC channel
	dac_channel_mask_t mask = DAC_CHANNEL_MASK_CH0;
	uint8_t channel = 0;

	if (config->pin == 25) {
		mask = DAC_CHANNEL_MASK_CH0;
		channel = 0;
	} else if (config->pin == 26) {
		mask = DAC_CHANNEL_MASK_CH1;
		channel = 1;
	}

	// If device handle is setup, first unsetup
	if ((_dac[channel].hndl != NULL) && (_dac[channel].device != NULL)) {
		tone_dac_unsetup(&_dac[channel].device);
	}

	// Allocate space for device handle
	*h = calloc(1, sizeof(tone_dac_device_t));

	if (!*h) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	driver_unit_lock_error_t *lock_error = NULL;

    if ((lock_error = driver_lock(SOUND_DRIVER, 0, GPIO_DRIVER, config->pin, DRIVER_ALL_FLAGS, NULL))) {
    	tone_dac_unsetup(h);

    	// Revoked lock on pin
    	return driver_lock_error(SOUND_DRIVER, lock_error);
    }
#endif

	(*h)->pin = config->pin;

	// Allocate space for internal buffer
#if CONFIG_DAC_DMA_AUTO_16BIT_ALIGN
	(*h)->buff_len = 1024;
#else
	(*h)->buff_len = 2048;
#endif

	(*h)->buff = calloc(1, (*h)->buff_len);
	if (!(*h)->buff) {
		tone_dac_unsetup(h);

		return driver_error(SOUND_DRIVER, SOUND_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Setup DAC
	dac_continuous_config_t dac_config = {
		.chan_mask = mask,
		.desc_num = 4,
		.buf_size = (*h)->buff_len * 2,
		.freq_hz = TONE_DAC_SAMPLE_RATE,
		.offset = 0,
		.clk_src = DAC_DIGI_CLK_SRC_APLL,
		.chan_mode = DAC_CHANNEL_MODE_SIMUL,
	};

	esp_err_t err;

	err = dac_continuous_new_channels(&dac_config, &_dac[(*h)->channel].hndl);
	if (err != ESP_OK) {
		tone_dac_unsetup(h);

		if (err == ESP_ERR_NO_MEM) {
			return driver_error(SOUND_DRIVER, SOUND_ERR_NOT_ENOUGH_MEMORY, NULL);
		} else {
			return driver_error(SOUND_DRIVER, SOUND_ERR_INTERNAL, NULL);
		}
	}

	dac_continuous_enable(_dac[(*h)->channel].hndl);

	(*h)->volume = 1.0;
	(*h)->samples = TONE_DAC_SAMPLE_RATE;
	(*h)->channel = channel;

	_dac[(*h)->channel].device = *h;

    return NULL;
}

void tone_dac_unsetup(tone_dac_device_h_t *h) {
	if (!*h) return;

	#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	driver_unlock(SOUND_DRIVER, 0, GPIO_DRIVER, (*h)->pin);
	#endif

	dac_continuous_disable(_dac[(*h)->channel].hndl);
	dac_continuous_del_channels(_dac[(*h)->channel].hndl);

	// Free resources
	if ((*h)->buff) {
		free((*h)->buff);
	}

	free(*h);
	*h = NULL;
}

driver_error_t *tone_dac_play(tone_dac_device_h_t *h, uint32_t freq, uint32_t duration) {
	if (!*h) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_NO_TONE_GEN, NULL);
	}

	/*
	 * The sine wave has the following equation:
	 *
	 * y(t) = sin(2 * PI * f * t + p) where:
	 *
	 * f: frequency
	 * t: time
	 * p: phase
	 *
	 * To sample the wave at a certain rate, t must be incremented
	 * in 1 / sample frequency increments, named delta.
	 *
	 * The idea is to sample the wave at the required sample frequency and,
	 * generate a buffer with the DAC values needed to reproduce the wave
	 * through the DAC controller.
	 *
	 * The wave can be sampled in an incremental way, so:
	 *
	 * sin(a + b) = sin(a) * cos(b) + sin(b) * cos(a)
	 * cos(a + b) = cos(a) * cos(b) + sin(a) * sin(b)
	 */

	// Convert duration from milliseconds to seconds
	double _duration = (double)(duration / 1000.0);

	double _period = 1.0 / (double)freq;
	double _curr_period = 0.0;

	double phase = 1.5 * M_PI;      // We start at 3/2 PI, to give a 0 DAC value
	double delta = (1.0 / ((double)(*h)->samples)); // Compute delta
	double y = 0.0;
	double t = 0.0;

	double _sin = sin(phase);
	double _cos = cos(phase);

	double _nsin;
	double _ncos;

	double tsin = sin(2.0 * M_PI * (double)freq * delta);
	double tcos = cos(2.0 * M_PI * (double)freq * delta);

	register int i = 0;

	do {
		y = map((*h)->volume * _sin, -1, 1, 0, 255);

#if CONFIG_DAC_DMA_AUTO_16BIT_ALIGN
		(*h)->buff[i++] = (uint8_t)y;
#else
		(*h)->buff[i++] = 0;
		(*h)->buff[i++] = (uint8_t)y;
#endif

		if (i == (*h)->buff_len) {
			dac_continuous_write(_dac[(*h)->channel].hndl, (*h)->buff, i, NULL, -1);
			i = 0;
		}

		_nsin = _sin * tcos + tsin * _cos;
		_ncos = _cos * tcos - _sin * tsin;

		_sin = _nsin;
		_cos = _ncos;

		t += delta;
		_curr_period += delta;

		if (_curr_period >= _period) {
			_curr_period = 0.0;
		}
	} while (t < _duration);

	while (_curr_period < _period) {
		y = map((*h)->volume * _sin, -1, 1, 0, 255);

#if CONFIG_DAC_DMA_AUTO_16BIT_ALIGN
		(*h)->buff[i++] = (uint8_t)y;
#else
		(*h)->buff[i++] = 0;
		(*h)->buff[i++] = (uint8_t)y;
#endif

		if (i == (*h)->buff_len) {
			dac_continuous_write(_dac[(*h)->channel].hndl, (*h)->buff, i, NULL, -1);
			i = 0;
		}

		_nsin = _sin * tcos + tsin * _cos;
		_ncos = _cos * tcos - _sin * tsin;

		_sin = _nsin;
		_cos = _ncos;

		t += delta;
		_curr_period += delta;
	}

	if (i > 0) {
		dac_continuous_write(_dac[(*h)->channel].hndl, (*h)->buff, i, NULL, -1);
	}

	return NULL;
}

driver_error_t *tone_dac_set_volume(tone_dac_device_h_t *h, float volume) {
	if (!*h) {
		return driver_error(SOUND_DRIVER, SOUND_ERR_NO_TONE_GEN, NULL);
	}

	(*h)->volume = (1.0 + log10(volume));

	return NULL;
}

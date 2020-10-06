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

#ifndef _SOUND_TONE_H_
#define _SOUND_TONE_H_

#include <stdint.h>

#include <sound/tone_pwm.h>
#include <sound/tone_dac.h>

#include <sys/driver.h>

typedef driver_error_t *(*tone_setup_t)(void *, void **);
typedef void (*tone_unsetup_t)(void **);
typedef driver_error_t *(*tone_play_t)(void **, uint32_t, uint32_t);
typedef driver_error_t *(*set_volume_t)(void **, float);

typedef enum {
	ToneGeneratorPWM = 1,
	ToneGeneratorDAC = 2,
	ToneGeneratorMAX
} tone_gen_t;

typedef union {
	tone_pwm_config_t pwm;
	tone_dac_config_t dac;
} tone_gen_config_t;

typedef struct {
	void **h;
	tone_unsetup_t _unsetup;
	tone_play_t    _play;
	set_volume_t   _set_volume;
} tone_gen_device_t;

typedef tone_gen_device_t *tone_gen_device_h_t;

driver_error_t *tone_setup(tone_gen_t, tone_gen_config_t *, tone_gen_device_h_t *);
driver_error_t *tone_unsetup(tone_gen_device_h_t *);
driver_error_t *tone_play(tone_gen_device_h_t *, uint32_t, uint32_t);
driver_error_t *tone_set_volume(tone_gen_device_h_t *h, float volume);

#endif /* _SOUND_TONE_H_ */

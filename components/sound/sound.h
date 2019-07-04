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
 * Lua RTOS, sound library
 *
 */

#ifndef _SOUND_H_
#define _SOUND_H_

#include <sound/tone.h>

#include <sys/driver.h>

void sound_music_time_signature(int upper, int lower, int bps);
driver_error_t *sound_music_note(char *note, int octave, tone_gen_device_h_t *h);
driver_error_t *sound_music_tone(int frequency, int duration, tone_gen_device_h_t *h);
driver_error_t *sound_music_silence(char *duration, tone_gen_device_h_t *h);


//  Driver errors
#define SOUND_ERR_NO_TONE_GEN              (DRIVER_EXCEPTION_BASE(SOUND_DRIVER_ID) |  0)
#define SOUND_ERR_INVALID_TONE_GEN         (DRIVER_EXCEPTION_BASE(SOUND_DRIVER_ID) |  1)
#define SOUND_ERR_INVALID_NOTE             (DRIVER_EXCEPTION_BASE(SOUND_DRIVER_ID) |  2)
#define SOUND_ERR_NOT_ENOUGH_MEMORY	 	   (DRIVER_EXCEPTION_BASE(SOUND_DRIVER_ID) |  4)
#define SOUND_ERR_INVALID_PIN              (DRIVER_EXCEPTION_BASE(SOUND_DRIVER_ID) |  5)
#define SOUND_ERR_NO_INVALID_VOLUME        (DRIVER_EXCEPTION_BASE(SOUND_DRIVER_ID) |  6)
#define SOUND_ERR_INVALID_SAMPLE_RATE      (DRIVER_EXCEPTION_BASE(SOUND_DRIVER_ID) |  7)

extern const int sound_errors;
extern const int sound_error_map;


#endif /* _SOUND_H_ */

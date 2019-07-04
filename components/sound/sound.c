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

#include "sdkconfig.h"
#include "sound.h"

#include <stdint.h>
#include <math.h>
#include <string.h>

#include <sys/driver.h>

#include <sys/delay.h>

DRIVER_REGISTER_BEGIN(SOUND,sound,0,NULL,NULL);
	DRIVER_REGISTER_ERROR(SOUND, sound, NoToneGen, "tone generator not setup", SOUND_ERR_NO_TONE_GEN);
	DRIVER_REGISTER_ERROR(SOUND, sound, InvalidToneGen, "invalid tone generator", SOUND_ERR_INVALID_TONE_GEN);
	DRIVER_REGISTER_ERROR(SOUND, sound, InvalidNote, "invalid note", SOUND_ERR_INVALID_NOTE);
	DRIVER_REGISTER_ERROR(SOUND, sound, NotEnoughtMemory, "not enough memory", SOUND_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(SOUND, sound, InvalidPin, "invalid pin", SOUND_ERR_INVALID_PIN);
	DRIVER_REGISTER_ERROR(SOUND, sound, InvalidVolume, "invalid volume", SOUND_ERR_NO_INVALID_VOLUME);
	DRIVER_REGISTER_ERROR(SOUND, sound, InvalidSampleRate, "invalid sample rate", SOUND_ERR_INVALID_SAMPLE_RATE);
DRIVER_REGISTER_END(SOUND,sound,0,NULL,NULL);

static int whole_duration;

/*
 * Helper functions
 */
static int music_note_duration(char *note) {
	// Convert to milliseconds
	int duration = whole_duration / atoi(note);

	if (*(note + strlen(note) - 1) == '.') {
		duration += duration * 0.5;
	}

	return duration;
}

/*
 * Operation functions
 */
void sound_music_time_signature(int upper, int lower, int bps) {
	// Compute whole note duration in milliseconds
	whole_duration = (60000 / bps) * lower;
}

driver_error_t *sound_music_note(char *note, int octave, tone_gen_device_h_t *h) {
	driver_error_t *error;

	// Extracted from https://pages.mtu.edu/~suits/NoteFreqCalcs.html
	//
	// 1) Parse note:
	//
	//    Note to play
	//    Detect half tones, +1 for # and -1 for b
	//    Get octave in which to play note
	//    Get duration of note
	//
	// 2) Compute the number of half steps away from A4
	//
	// 3) Compute the frequency of the note, applying the following formula:
	//
	//    frequency = 440 * 1.059463094359 ^ half_steps
	//
	// 4) Play tone at the computed frequency and duration
	int8_t half_tone;
	int8_t half_steps;
	int duration;

	if (strlen(note) <= 5) {
		if ((*note >= 'A') && (*note <= 'G')) {
			if (*(note + 1) == '#') {
				half_tone = 1;
				duration = music_note_duration(note + 2);
			} else if (*(note + 1) == 'b') {
				half_tone = -1;
				duration = music_note_duration(note + 2);
			} else {
				half_tone = 0;
				duration = music_note_duration(note + 1);
			}

			switch (*note) {
				case 'A':
					half_steps = 12 * (octave - 4) + half_tone;
					break;
				case 'B':
					half_steps = 12 * (octave - 4) + 2 + half_tone;
					break;
				case 'C':
					half_steps = 12 * (octave - 5) + 3 + half_tone;
					break;
				case 'D':
					half_steps = 12 * (octave - 5) + 5 + half_tone;
					break;
				case 'E':
					half_steps = 12 * (octave - 5) + 7 + half_tone;
					break;
				case 'F':
					half_steps = 12 * (octave - 5) + 8 + half_tone;
					break;
				case 'G':
					half_steps = 12 * (octave - 5) + 10 + half_tone;
					break;
			}

			error = tone_play(h, round(440.0  * pow(1.059463094359, half_steps)), duration);
			if (error) {
				return error;
			}
		} else {
			// Invalid note
			return driver_error(SOUND_DRIVER, SOUND_ERR_INVALID_NOTE, NULL);
		}
	} else {
		// Invalid note
		return driver_error(SOUND_DRIVER, SOUND_ERR_INVALID_NOTE, NULL);
	}

	return NULL;
}

driver_error_t *sound_music_tone(int frequency, int duration, tone_gen_device_h_t *h) {
	driver_error_t *error;

	error = tone_play(h, frequency, duration);
	if (error) {
		return error;
	}

	return NULL;
}

driver_error_t *sound_music_silence(char *duration, tone_gen_device_h_t *h) {
	int _duration = music_note_duration(duration);

	delay(_duration);

	return NULL;
}

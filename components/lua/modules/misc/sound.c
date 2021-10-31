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
 * Lua RTOS, Lua SOUND module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SOUND

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "error.h"
#include "sound.h"
#include "modules.h"

#include <sound/sound.h>

typedef struct {
	tone_gen_device_h_t h;
} sound_userdata;

static int lsound_attach(lua_State* L) {
	int tone_generator = luaL_checkinteger(L, 1);
	int pin = luaL_checkinteger(L, 2);

	// Setup tone generator
	tone_gen_config_t config;

	if (tone_generator == ToneGeneratorPWM) {
		config.pwm.unit = 0;
		config.pwm.pin = pin;
	} else if (tone_generator == ToneGeneratorDAC) {
		config.dac.pin = pin;
	}

	sound_userdata *sound = (sound_userdata *)lua_newuserdata(L, sizeof(sound_userdata));
	sound->h = NULL;

	driver_error_t *error;

	if ((error = tone_setup(tone_generator, &config, &sound->h))) {
    	return luaL_driver_error(L, error);
	}

	// Set default time signature
	sound_music_time_signature(4, 4, 120);

    luaL_getmetatable(L, "sound.device");
    lua_setmetatable(L, -2);

    return 1;
}

static int lsound_detach(lua_State* L) {
    driver_error_t *error;
    sound_userdata *sound = NULL;

    sound = (sound_userdata *) luaL_testudata(L, 1, "sound.device");

    if (sound) {
        if ((error = tone_unsetup(&sound->h))) {
            return luaL_driver_error(L, error);
        }
    }

    return 0;
}

static int lsound_music_note(lua_State* L) {
    sound_userdata *sound = (sound_userdata *)luaL_checkudata(L, 1, "sound.device");
    luaL_argcheck(L, sound, 1, "sound expected");

    char *note = (char *)luaL_checkstring(L, 2);
	int octave = luaL_checkinteger(L, 3);

    driver_error_t *error;

	if ((error = sound_music_note(note, octave, &sound->h))) {
    	return luaL_driver_error(L, error);
	}

    return 0;
}

static int lsound_music_tone(lua_State* L) {
    sound_userdata *sound = (sound_userdata *)luaL_checkudata(L, 1, "sound.device");
    luaL_argcheck(L, sound, 1, "sound expected");

    int frequency = luaL_checkinteger(L, 2);
	int duration = luaL_checkinteger(L, 3);

    driver_error_t *error;

	if ((error = sound_music_tone(frequency, duration, &sound->h))) {
    	return luaL_driver_error(L, error);
	}

    return 0;
}

static int lsound_music_silence(lua_State* L) {
    sound_userdata *sound = (sound_userdata *)luaL_checkudata(L, 1, "sound.device");
    luaL_argcheck(L, sound, 1, "sound expected");

    char *duration = (char *)luaL_checkstring(L, 2);

    driver_error_t *error;

	if ((error = sound_music_silence(duration, &sound->h))) {
    	return luaL_driver_error(L, error);
	}

    return 0;
}

static int lsound_time_signature(lua_State* L) {
    sound_userdata *sound = (sound_userdata *)luaL_checkudata(L, 1, "sound.device");
    luaL_argcheck(L, sound, 1, "sound expected");

    int upper = luaL_checkinteger(L, 2);
	int lower = luaL_checkinteger(L, 3);
	int bps   = luaL_checkinteger(L, 4);

	sound_music_time_signature(upper, lower, bps);

	return 0;
}

static int lsound_setvolume(lua_State* L) {
    sound_userdata *sound = (sound_userdata *)luaL_checkudata(L, 1, "sound.device");
    luaL_argcheck(L, sound, 1, "sound expected");

    float volume = luaL_checknumber(L, 2);

    driver_error_t *error;

	if ((error = tone_set_volume(&sound->h, volume))) {
    	return luaL_driver_error(L, error);
	}

	return 0;
}

// Destructor
static int lsound_trans_gc(lua_State *L) {
	return lsound_detach(L);
}

static const LUA_REG_TYPE lsound_map[] = {
    { LSTRKEY( "attach"        ),     LFUNCVAL( lsound_attach          ) },

	{LSTRKEY("PWM"), LINTVAL(ToneGeneratorPWM)},
	{LSTRKEY("DAC"), LINTVAL(ToneGeneratorDAC)},

	{ LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lsound_device_map[] = {
	{ LSTRKEY( "setvolume"     ),     LFUNCVAL( lsound_setvolume       ) },
	{ LSTRKEY( "detach"        ),     LFUNCVAL( lsound_detach          ) },
	{ LSTRKEY( "timesignature" ),	  LFUNCVAL( lsound_time_signature  ) },
  	{ LSTRKEY( "playnote"      ),	  LFUNCVAL( lsound_music_note      ) },
  	{ LSTRKEY( "playtone"      ),	  LFUNCVAL( lsound_music_tone      ) },
  	{ LSTRKEY( "playsilence"   ),	  LFUNCVAL( lsound_music_silence   ) },
    { LSTRKEY( "__metatable"   ),	  LROVAL  ( lsound_device_map      ) },
	{ LSTRKEY( "__index"       ),     LROVAL  ( lsound_device_map      ) },
    { LSTRKEY( "__gc"          ),     LFUNCVAL( lsound_trans_gc        ) },
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_sound( lua_State *L ) {
    luaL_newmetarotable(L,"sound.device", (void *)lsound_device_map);
    return 0;
}

MODULE_REGISTER_ROM(SOUND, sound, lsound_map, luaopen_sound, 1);

#endif


/*

buzzer = sound.attach(sound.DAC, pio.GPIO26)

buzzer:timesignature(3,4,240)

buzzer:playnote("B4",   4)
buzzer:playnote("E4.",  5)
buzzer:playnote("G8",   5)
buzzer:playnote("F#4",  5)
buzzer:playnote("E2",   5)
buzzer:playnote("B4",   5)
buzzer:playnote("A2.",  5)
buzzer:playnote("F#2.", 5)
buzzer:playnote("E4.",  5)
buzzer:playnote("G8",   5)
buzzer:playnote("F#4",  5)
buzzer:playnote("D2",   5)
buzzer:playnote("F4",   5)
buzzer:playnote("B2.",  4)

 */

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
 * Lua RTOS, Lua PWM module
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_PWM

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "pwm.h"
#include "error.h"
#include "modules.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <drivers/cpu.h>
#include <drivers/pwm.h>

static int lpwm_attach( lua_State* L ) {
	driver_error_t *error;
    int8_t pin;
    int32_t freq;
    double duty;

    pwm_userdata *pwm = (pwm_userdata *)lua_newuserdata(L, sizeof(pwm_userdata));

    pwm->unit = 0;

    pin = luaL_checkinteger( L, 1 );
    freq = luaL_checkinteger( L, 2 );
    duty = luaL_checknumber(L, 3);

    if ((error = pwm_setup(pwm->unit, -1, pin, freq, duty, &pwm->channel))) {
    	return luaL_driver_error(L, error);
    }

    luaL_getmetatable(L, "pwm.inst");
    lua_setmetatable(L, -2);

    return 1;
}

static int lpwm_detach( lua_State* L ) {
    pwm_userdata *pwm = NULL;
	driver_error_t *error;

    pwm = (pwm_userdata *)luaL_checkudata(L, 1, "pwm.inst");
    luaL_argcheck(L, pwm, 1, "pwm expected");

    if ((error = pwm_unsetup(pwm->unit, pwm->channel))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}


static int lpwm_setfreq(lua_State* L) {
    pwm_userdata *pwm = NULL;
	driver_error_t *error;
	int32_t freq;

    pwm = (pwm_userdata *)luaL_checkudata(L, 1, "pwm.inst");
    luaL_argcheck(L, pwm, 1, "pwm expected");

    freq = luaL_checknumber(L, 2);

    if ((error = pwm_set_freq(pwm->unit, pwm->channel, freq))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int lpwm_setduty(lua_State* L) {
    pwm_userdata *pwm = NULL;
	driver_error_t *error;
	double duty;

    pwm = (pwm_userdata *)luaL_checkudata(L, 1, "pwm.inst");
    luaL_argcheck(L, pwm, 1, "pwm expected");

    duty = luaL_checknumber(L, 2);

    if ((error = pwm_set_duty(pwm->unit, pwm->channel, duty))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int lpwm_start(lua_State* L) {
    pwm_userdata *pwm = NULL;
	driver_error_t *error;

    pwm = (pwm_userdata *)luaL_checkudata(L, 1, "pwm.inst");
    luaL_argcheck(L, pwm, 1, "pwm expected");

    if ((error = pwm_start(pwm->unit, pwm->channel))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int lpwm_stop(lua_State* L) {
    pwm_userdata *pwm = NULL;
	driver_error_t *error;

    pwm = (pwm_userdata *)luaL_checkudata(L, 1, "pwm.inst");
    luaL_argcheck(L, pwm, 1, "pwm expected");

    if ((error = pwm_stop(pwm->unit, pwm->channel))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

// Destructor
static int lpwm_gc (lua_State *L) {
	lpwm_detach(L);

	return 0;
}

static const LUA_REG_TYPE lpwm_map[] = {
    { LSTRKEY("attach" ),	LFUNCVAL(lpwm_attach)    },
	PWM_PWM0
	PWM_PWM1
	PWM_PWM_CH0
	PWM_PWM_CH1
	PWM_PWM_CH2
	PWM_PWM_CH3
	PWM_PWM_CH4
	PWM_PWM_CH5
	PWM_PWM_CH6
	PWM_PWM_CH7
	PWM_PWM_CH8
	PWM_PWM_CH9
	PWM_PWM_CH10
	PWM_PWM_CH11
	PWM_PWM_CH12
	PWM_PWM_CH13
	PWM_PWM_CH14
	PWM_PWM_CH15
	DRIVER_REGISTER_LUA_ERRORS(pwm)
  	{ LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lpwm_channel_map[] = {
  { LSTRKEY( "setduty"        ),	 LFUNCVAL( lpwm_setduty                ) },
  { LSTRKEY( "setfreq"        ),	 LFUNCVAL( lpwm_setfreq                ) },
  { LSTRKEY( "start"          ),	 LFUNCVAL( lpwm_start                  ) },
  { LSTRKEY( "stop"           ),	 LFUNCVAL( lpwm_stop                   ) },
  { LSTRKEY( "detach"         ),	 LFUNCVAL( lpwm_detach                 ) },
  { LSTRKEY( "__metatable"    ),	 LROVAL  ( lpwm_channel_map            ) },
  { LSTRKEY( "__index"        ),   	 LROVAL  ( lpwm_channel_map            ) },
  { LSTRKEY( "__gc"           ),	 LFUNCVAL( lpwm_gc 	                   ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_pwm( lua_State *L ) {
    luaL_newmetarotable(L,"pwm.inst", (void *)lpwm_channel_map);
    return 0;
}

MODULE_REGISTER_MAPPED(PWM, pwm, lpwm_map, luaopen_pwm);

#endif

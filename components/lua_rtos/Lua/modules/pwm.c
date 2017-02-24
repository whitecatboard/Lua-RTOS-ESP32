/*
 * Lua RTOS, PWM Lua module
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "luartos.h"

#if LUA_USE_PWM

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

extern const LUA_REG_TYPE pwm_error_map[];

static int lpwm_setup( lua_State* L ) {
	driver_error_t *error;
    int8_t id;

    id = luaL_checkinteger( L, 1 );

    pwm_userdata *pwm = (pwm_userdata *)lua_newuserdata(L, sizeof(pwm_userdata));

    pwm->unit = id;
    pwm->channel = -1;

    if ((error = pwm_setup(id))) {
    	return luaL_driver_error(L, error);
    }

    luaL_getmetatable(L, "pwm");
    lua_setmetatable(L, -2);

    return 1;
}

static int lpwm_setup_channel( lua_State* L ) {
    int8_t channel, pin;
    int32_t freq;
    double duty;

    pwm_userdata *pwm = NULL;
	driver_error_t *error;

    pwm = (pwm_userdata *)luaL_checkudata(L, 1, "pwm");
    luaL_argcheck(L, pwm, 1, "pwm expected");

    channel = luaL_checkinteger( L, 2 );
    pin = luaL_checkinteger( L, 3 );
    freq = luaL_checkinteger( L, 4 );
    duty = luaL_checknumber(L, 5);

    if ((error = pwm_setup_channel(pwm->unit, channel, pin, freq, duty, &channel))) {
    	return luaL_driver_error(L, error);
    }

    pwm_userdata *npwm = (pwm_userdata *)lua_newuserdata(L, sizeof(pwm_userdata));
    memcpy(npwm, pwm, sizeof(pwm_userdata));

    npwm->channel = channel;

    luaL_getmetatable(L, "pwm");
    lua_setmetatable(L, -2);

    return 1;
}

static int lpwm_setduty(lua_State* L) {
    pwm_userdata *pwm = NULL;
	driver_error_t *error;
	double duty;

    pwm = (pwm_userdata *)luaL_checkudata(L, 1, "pwm");
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

    pwm = (pwm_userdata *)luaL_checkudata(L, 1, "pwm");
    luaL_argcheck(L, pwm, 1, "pwm expected");

    if ((error = pwm_start(pwm->unit, pwm->channel))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int lpwm_stop(lua_State* L) {
    pwm_userdata *pwm = NULL;
	driver_error_t *error;

    pwm = (pwm_userdata *)luaL_checkudata(L, 1, "pwm");
    luaL_argcheck(L, pwm, 1, "pwm expected");

    if ((error = pwm_stop(pwm->unit, pwm->channel))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int lpwm_index(lua_State *L);
static int lpwm_channel_index(lua_State *L);

static const LUA_REG_TYPE lpwm_map[] = {
    { LSTRKEY("setup" )     ,	LFUNCVAL(lpwm_setup)     },
  	{ LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lpwm_constants_map[] = {
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

	// Error definitions
	{LSTRKEY("error"),  LROVAL( pwm_error_map )},

	{ LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lpwm_channel_map[] = {
  { LSTRKEY( "setupchan"      ),	 LFUNCVAL( lpwm_setup_channel          ) },
  { LSTRKEY( "setduty"        ),	 LFUNCVAL( lpwm_setduty                ) },
  { LSTRKEY( "start"          ),	 LFUNCVAL( lpwm_start                  ) },
  { LSTRKEY( "stop"           ),	 LFUNCVAL( lpwm_stop                   ) },
  { LNILKEY, LNILVAL }
};

static const luaL_Reg lpwm_func[] = {
    { "__index"    , 	lpwm_index },
    { NULL, NULL }
};

static const luaL_Reg lpwm_channel_func[] = {
    { "__index", 	lpwm_channel_index },
    { NULL, NULL }
};

static int lpwm_index(lua_State *L) {
	return luaR_index(L, lpwm_map, lpwm_constants_map);
}

static int lpwm_channel_index(lua_State *L) {
	return luaR_index(L, lpwm_channel_map, NULL);
}

LUALIB_API int luaopen_pwm( lua_State *L ) {
    luaL_newlib(L, lpwm_func);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);

    luaL_newmetatable(L, "pwm");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    luaL_setfuncs(L, lpwm_channel_func, 0);
    lua_pop(L, 1);

    return 1;
}

MODULE_REGISTER_UNMAPPED(PWM, pwm, luaopen_pwm);

#endif

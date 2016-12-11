/*
 * Lua RTOS, PWM Lua module
 *
 * Copyright (C) 2015 - 2016
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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <drivers/cpu.h>
#include <drivers/pwm.h>

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

    if ((error = pwm_setup_channel(pwm->unit, channel, pin, freq, duty))) {
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

#include "modules.h"

static const LUA_REG_TYPE pwm_method_map[] = {
  { LSTRKEY( "setupchan"      ),	 LFUNCVAL( lpwm_setup_channel          ) },
  { LSTRKEY( "setduty"        ),	 LFUNCVAL( lpwm_setduty                ) },
  { LSTRKEY( "start"          ),	 LFUNCVAL( lpwm_start                  ) },
  { LSTRKEY( "stop"           ),	 LFUNCVAL( lpwm_stop                   ) },
  { LNILKEY, LNILVAL }
};

#if LUA_USE_ROTABLE

static const LUA_REG_TYPE pwm_constants_map[] = {
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
	{ LNILKEY, LNILVAL }
};

static int luaL_pwm_index(lua_State *L) {
	int res;

	if ((res = luaR_findfunction(L, pwm_method_map)) != 0)
		return res;

	const char *key = luaL_checkstring(L, 2);
	const TValue *val = luaR_findentry(pwm_constants_map, key, 0, NULL);
	if (val != luaO_nilobject) {
		lua_pushinteger(L, val->value_.i);
		return 1;
	}

	return (int)luaO_nilobject;
}

static const luaL_Reg pwm_load_funcs[] = {
    { "__index"    , 	luaL_pwm_index },
    { "setup"      ,	lpwm_setup     },
    { NULL, NULL }
};

static int luaL_mpwm_index(lua_State *L) {
  int fres;
  if ((fres = luaR_findfunction(L, pwm_method_map)) != 0)
    return fres;

  return (int)luaO_nilobject;
}

static const luaL_Reg mpwm_load_funcs[] = {
    { "__index"    , 	luaL_mpwm_index },
    { NULL, NULL }
};

#else

static const luaL_Reg pwm_map[] = {
	{ "setup"      ,	lpwm_setup     },
	{ NULL, NULL }
};

#endif

LUALIB_API int luaopen_pwm( lua_State *L ) {
#if !LUA_USE_ROTABLE
    int i;
    char buff[5];

    luaL_register( L, AUXLIB_PWM, pwm_map );

    // Set it as its own metatable
    lua_pushvalue( L, -1 );
    lua_setmetatable( L, -2 );

    // create metatable for adc module
    luaL_newmetatable(L, "pwm");

    // Module constants
    for(i=CPU_FIRST_PWM;i<=CPU_FIRST_PWM;i++) {
        sprintf(buff,"PWM%d",i);
        MOD_REG_INTEGER( L, buff, i );
    }

    for(i=CPU_FIRST_PWM_CH;i<=CPU_LAST_PWM_CH;i++) {
        sprintf(buff,"PWM_CH%d",i);
        MOD_REG_INTEGER( L, buff, i );
    }

    lua_pushliteral(L, "__index");
    lua_pushvalue(L,-2);
    lua_rawset(L,-3);

    // Setup the methods inside metatable
    luaL_register( L, NULL, pwm_method_map );
#else
    luaL_newlib(L, pwm_load_funcs);  /* new module */
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);

    luaL_newmetatable(L, "pwm");  /* create metatable */
    lua_pushvalue(L, -1);  /* push metatable */
    lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */

    luaL_setfuncs(L, mpwm_load_funcs, 0);  /* add file methods to new metatable */
    lua_pop(L, 1);  /* pop new metatable */
#endif

    return 1;
}

#endif

/*
 * Lua RTOS, Lua servo module
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
 * and fitness.  In no servo shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "luartos.h"

#if LUA_USE_SERVO

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"
#include "servo.h"
#include "modules.h"

#include <drivers/servo.h>

// This variables are defined at linker time
extern LUA_REG_TYPE servo_error_map[];

static int lservo_attach( lua_State* L ) {
	driver_error_t *error;

	// Create user data
    servo_userdata *udata = (servo_userdata *)lua_newuserdata(L, sizeof(servo_userdata));
    if (!udata) {
    	return 0;
    }

    int8_t pin = luaL_checkinteger(L, 1);

    if ((error = servo_setup(pin, &udata->instance))) {
    	return luaL_driver_error(L, error);
    }

    luaL_getmetatable(L, "servo");
    lua_setmetatable(L, -2);

    return 1;
}

static int lservo_write( lua_State* L ) {
	driver_error_t *error;
	servo_userdata *udata;

	// Get user data
	udata = (servo_userdata *)luaL_checkudata(L, 1, "servo");
	luaL_argcheck(L, udata, 1, "servo expected");

    double value = luaL_checknumber(L, 2);

    if ((error = servo_write(udata->instance, value))) {
    	return luaL_driver_error(L, error);
    }

	return 0;
}

// Destructor
static int lservo_ins_gc (lua_State *L) {
    servo_userdata *udata = NULL;

    udata = (servo_userdata *)luaL_checkudata(L, 1, "servo");
	if (udata) {
		free(udata->instance);
	}

	return 0;
}

static int lservo_index(lua_State *L);
static int lservo_ins_index(lua_State *L);

static const LUA_REG_TYPE lservo_map[] = {
    { LSTRKEY( "attach"  ),	LFUNCVAL( lservo_attach ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lservo_ins_map[] = {
    { LSTRKEY( "write"   ),	LFUNCVAL( lservo_write      ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lservo_constants_map[] = {
	{LSTRKEY("error"), 			 LROVAL( servo_error_map )},
	{ LNILKEY, LNILVAL }
};

static const luaL_Reg lservo_func[] = {
    { "__index", 	lservo_index },
    { NULL, NULL }
};

static const luaL_Reg lservo_ins_func[] = {
	{ "__gc"   , 	lservo_ins_gc },
    { "__index", 	lservo_ins_index },
    { NULL, NULL }
};

static int lservo_index(lua_State *L) {
	return luaR_index(L, lservo_map, lservo_constants_map);
}

static int lservo_ins_index(lua_State *L) {
	return luaR_index(L, lservo_ins_map, NULL);
}

LUALIB_API int luaopen_servo( lua_State *L ) {
	luaL_newlib(L, lservo_func);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);

    luaL_newmetatable(L, "servo");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    luaL_setfuncs(L, lservo_ins_func, 0);
    lua_pop(L, 1);

    return 1;
}

MODULE_REGISTER_UNMAPPED(SERVO, servo, luaopen_servo);

#endif

/*

s1 = servo.attach(pio.GPIO4)
s1:move(45)

*/

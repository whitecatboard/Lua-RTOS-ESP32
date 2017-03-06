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

    luaL_getmetatable(L, "servo.ins");
    lua_setmetatable(L, -2);

    return 1;
}

static int lservo_write( lua_State* L ) {
	driver_error_t *error;
	servo_userdata *udata;

	// Get user data
	udata = (servo_userdata *)luaL_checkudata(L, 1, "servo.ins");
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

    udata = (servo_userdata *)luaL_checkudata(L, 1, "servo.ins");
	if (udata) {
		free(udata->instance);
	}

	return 0;
}

static const LUA_REG_TYPE lservo_map[] = {
    { LSTRKEY( "attach"  ),			LFUNCVAL( lservo_attach ) },
	{ LSTRKEY ("error"   ), 		LROVAL( servo_error_map )},
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lservo_ins_map[] = {
    { LSTRKEY( "write"        ),	LFUNCVAL( lservo_write   ) },
	{ LSTRKEY( "__metatable"  ),    LROVAL  ( lservo_ins_map ) },
	{ LSTRKEY( "__index"      ),   	LROVAL  ( lservo_ins_map ) },
	{ LSTRKEY( "__gc"         ),   	LROVAL  ( lservo_ins_gc ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_servo( lua_State *L ) {
    luaL_newmetarotable(L,"servo.ins", (void *)lservo_ins_map);
    return 0;
}

MODULE_REGISTER_MAPPED(SERVO, servo, lservo_map, luaopen_servo);

#endif

/*

s1 = servo.attach(pio.GPIO4)
s1:move(45)

*/

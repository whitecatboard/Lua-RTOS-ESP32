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
 * Lua RTOS, Lua servo module
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SERVO

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"
#include "servo.h"
#include "modules.h"

#include <drivers/servo.h>

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
	DRIVER_REGISTER_LUA_ERRORS(servo)
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lservo_ins_map[] = {
    { LSTRKEY( "write"        ),	LFUNCVAL( lservo_write   ) },
	{ LSTRKEY( "__metatable"  ),    LROVAL  ( lservo_ins_map ) },
	{ LSTRKEY( "__index"      ),   	LROVAL  ( lservo_ins_map ) },
	{ LSTRKEY( "__gc"         ),   	LFUNCVAL( lservo_ins_gc ) },
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

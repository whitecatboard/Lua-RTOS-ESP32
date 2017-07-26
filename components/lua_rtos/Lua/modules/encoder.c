/*
 * Lua RTOS, encoder MODULE
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_ENCODER

#include "freertos/FreeRTOS.h"
#include "freertos/adds.h"

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"
#include "error.h"

#include <drivers/encoder.h>

typedef struct {
	encoder_h_t *encoder;
	int callback;
} encoder_userdata;

static void callback_func(int callback, uint32_t value, uint8_t button) {
	lua_State *TL;
	lua_State *L;
	int tref;

	if (callback != LUA_NOREF) {
	    L = pvGetLuaState();
	    TL = lua_newthread(L);

	    tref = luaL_ref(L, LUA_REGISTRYINDEX);

	    lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
	    lua_xmove(L, TL, 1);
        lua_pushinteger(TL, value);
        lua_pushinteger(TL, button);
	    lua_pcall(TL, 2, 0, 0);
        luaL_unref(TL, LUA_REGISTRYINDEX, tref);
	}
}

static int lencoder_attach( lua_State* L ) {
	driver_error_t *error;
	int sw;
	int callback;

	int a = luaL_checkinteger(L, 1);
	int b = luaL_checkinteger(L, 2);

	if (lua_isnil(L, 3)) {
		sw = -1;
	} else {
		sw = luaL_checkinteger(L, 3);
	}

	if (lua_isfunction(L, 4)) {
		luaL_checktype(L, 4, LUA_TFUNCTION);
		lua_pushvalue(L, 4);

		callback = luaL_ref(L, LUA_REGISTRYINDEX);
	} else {
		callback = LUA_NOREF;
	}

	encoder_userdata *userdata = (encoder_userdata *)lua_newuserdata(L, sizeof(encoder_userdata));
    if (!userdata) {
       	return luaL_exception(L, ENCODER_ERR_NOT_ENOUGH_MEMORY);
    }

    // Setup the encoder
    encoder_h_t *encoder;
    if ((error = encoder_setup(a, b, sw, &encoder))) {
    	return luaL_driver_error(L, error);
    }

    if (callback != LUA_NOREF) {
        // Register callback, the id of the callback is the callback reference
        if ((error = encoder_register_callback(encoder, callback_func, callback, 1))) {
        	return luaL_driver_error(L, error);
        }
    }

    userdata->callback = callback;
    userdata->encoder = encoder;

    luaL_getmetatable(L, "encoder.enc");
    lua_setmetatable(L, -2);

    return 1;
}

static int lencoder_read (lua_State *L) {
	encoder_userdata *userdata = (encoder_userdata *)luaL_checkudata(L, 1, "encoder.enc");
	driver_error_t *error;
	int32_t val;
	uint8_t button;

    if ((error = encoder_read(userdata->encoder, &val, &button))) {
    	return luaL_driver_error(L, error);
    }

    lua_pushinteger(L, val);
    lua_pushinteger(L, button);

	return 2;
}

static int lencoder_write(lua_State *L) {
	encoder_userdata *userdata = (encoder_userdata *)luaL_checkudata(L, 1, "encoder.enc");
	driver_error_t *error;

	int32_t val = luaL_checkinteger(L, 1);

    if ((error = encoder_write(userdata->encoder, val))) {
    	return luaL_driver_error(L, error);
    }

    lua_pushinteger(L, val);

	return 1;
}

static int lencoder_detach (lua_State *L) {
	encoder_userdata *userdata = (encoder_userdata *)luaL_checkudata(L, 1, "encoder.enc");
	driver_error_t *error;

    if ((error = encoder_unsetup(userdata->encoder))) {
    	return luaL_driver_error(L, error);
    }

	luaL_unref(L, LUA_REGISTRYINDEX, userdata->callback);

	return 0;
}

// Destructor
static int lencoder_gc (lua_State *L) {
	lencoder_detach(L);

	return 0;
}

static const LUA_REG_TYPE encoder_inst_map[] = {
	{ LSTRKEY( "detach" ),			LFUNCVAL( lencoder_detach  ) },
	{ LSTRKEY( "read" ),			LFUNCVAL( lencoder_read    ) },
	{ LSTRKEY( "write" ),			LFUNCVAL( lencoder_write   ) },
    { LSTRKEY( "__metatable" ),	    LROVAL  ( encoder_inst_map ) },
	{ LSTRKEY( "__index"     ),     LROVAL  ( encoder_inst_map ) },
    { LSTRKEY( "__gc" ),	 	    LROVAL  ( lencoder_gc 	   ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE encoder_map[] = {
	{ LSTRKEY( "attach" ),			LFUNCVAL( lencoder_attach ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_encoder( lua_State *L ) {
    luaL_newmetarotable(L,"encoder.enc", (void *)encoder_inst_map);
	return 0;
}

MODULE_REGISTER_MAPPED(ENCODER, encoder, encoder_map, luaopen_encoder);

/*

function new_pos(value, button)
   if (button == 0) then
      print("push")
   else
      print(value)
   end
end

enc = encoder.attach(pio.GPIO26, pio.GPIO14, pio.GPIO21, new_pos)

enc:read()
enc:write(10)

 */
#endif

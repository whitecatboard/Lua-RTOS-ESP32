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
 * Lua RTOS, Lua encoder module
 *
 */
#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_ENCODER

#include "freertos/FreeRTOS.h"
#include "freertos/adds.h"

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"
#include "error.h"
#include "sys.h"

#include <drivers/encoder.h>

typedef struct {
	encoder_h_t *encoder;
	lua_callback_t *callback;
} encoder_userdata;

static void callback_func(int callback, int8_t dir, uint32_t counter, uint8_t button) {
	lua_callback_t *cb = (lua_callback_t *)callback;

	if (cb != NULL) {
		lua_State *state = luaS_callback_state(cb);

		if (state != NULL) {
	        lua_pushinteger(state, dir);
	        lua_pushinteger(state, counter);
	        lua_pushinteger(state, button);
	        luaS_callback_call(cb, 3);
		}
	}
}

static int lencoder_attach( lua_State* L ) {
	lua_callback_t *callback;
	driver_error_t *error;
	int sw;

	int a = luaL_checkinteger(L, 1);
	int b = luaL_checkinteger(L, 2);

	if (lua_isnil(L, 3)) {
		sw = -1;
	} else {
		sw = luaL_checkinteger(L, 3);
	}

	if (lua_isfunction(L, 4)) {
		luaL_checktype(L, 4, LUA_TFUNCTION);

		callback = luaS_callback_create(L, 4);
	} else {
		callback = NULL;
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

    if (callback != NULL) {
        // Register callback, the id of the callback is the callback reference
        if ((error = encoder_register_callback(encoder, callback_func, (int)callback, 1))) {
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

    if (userdata->callback != NULL) {
    	luaS_callback_destroy(userdata->callback);
    }

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
    { LSTRKEY( "__gc" ),	 	    LFUNCVAL( lencoder_gc 	   ) },
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

MODULE_REGISTER_ROM(ENCODER, encoder, encoder_map, luaopen_encoder, 1);

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

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
 * Lua RTOS, Lua neopixel module
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_NEOPIXEL

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"
#include "neopixel.h"
#include "modules.h"

#include <stdio.h>
#include <string.h>

#include <drivers/neopixel.h>

static int lneopixel_setup( lua_State* L ) {
    int type, gpio, pixels;
	driver_error_t *error;

	luaL_deprecated(L, "neopixel.setup", "neopixel.attach");

    type = luaL_checkinteger( L, 1 );
    gpio = luaL_checkinteger( L, 2 );
    pixels = luaL_checkinteger( L, 3 );

    neopixel_userdata *neopixel = (neopixel_userdata *)lua_newuserdata(L, sizeof(neopixel_userdata));

    if ((error = neopixel_setup(type, gpio, pixels, &neopixel->unit))) {
    	return luaL_driver_error(L, error);
    }

    luaL_getmetatable(L, "neopixel.inst");
    lua_setmetatable(L, -2);

    return 1;
}

static int lneopixel_attach( lua_State* L ) {
    int type, gpio, pixels;
	driver_error_t *error;

    type = luaL_checkinteger( L, 1 );
    gpio = luaL_checkinteger( L, 2 );
    pixels = luaL_checkinteger( L, 3 );

    neopixel_userdata *neopixel = (neopixel_userdata *)lua_newuserdata(L, sizeof(neopixel_userdata));

    if ((error = neopixel_setup(type, gpio, pixels, &neopixel->unit))) {
    	return luaL_driver_error(L, error);
    }

    luaL_getmetatable(L, "neopixel.inst");
    lua_setmetatable(L, -2);

    return 1;
}

static int lneopixel_set_pixel( lua_State* L ) {
	driver_error_t *error;
	neopixel_userdata *neopixel = NULL;

	neopixel = (neopixel_userdata *)luaL_checkudata(L, 1, "neopixel.inst");
    luaL_argcheck(L, neopixel, 1, "neopixel expected");

    int pixel = luaL_checkinteger( L, 2 );
    int r = luaL_checkinteger( L, 3 );
    int g = luaL_checkinteger( L, 4 );
    int b = luaL_checkinteger( L, 5 );

    if (pixel < 0) {
    	return luaL_exception(L, NEOPIXEL_ERR_INVALID_PIXEL);
    }

    if ((r < 0) || (r > 255)) {
    	return luaL_exception_extended(L, NEOPIXEL_ERR_INVALID_RGB_COMPONENT, "r");
    }

    if ((g < 0) || (g > 255)) {
    	return luaL_exception_extended(L, NEOPIXEL_ERR_INVALID_RGB_COMPONENT, "g");
    }

    if ((b < 0) || (b > 255)) {
    	return luaL_exception_extended(L, NEOPIXEL_ERR_INVALID_RGB_COMPONENT, "b");
    }

    if ((error = neopixel_rgb(neopixel->unit, pixel, r, g, b))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int lneopixel_update( lua_State* L ) {
	driver_error_t *error;
	neopixel_userdata *neopixel = NULL;

	neopixel = (neopixel_userdata *)luaL_checkudata(L, 1, "neopixel.inst");
    luaL_argcheck(L, neopixel, 1, "neopixel expected");

    if ((error = neopixel_update(neopixel->unit))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static const LUA_REG_TYPE lneopixel_map[] = {
    { LSTRKEY( "setup"   ),	     LFUNCVAL ( lneopixel_setup    ) },
    { LSTRKEY( "attach"  ),	     LFUNCVAL ( lneopixel_attach   ) },
	DRIVER_REGISTER_LUA_ERRORS(neopixel)
	{ LSTRKEY( "WS2812B" ),      LINTVAL  ( NeopixelWS2812B    ) },
	{ LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lneopixel_inst_map[] = {
	{ LSTRKEY( "setPixel"    ),	  LFUNCVAL( lneopixel_set_pixel     ) },
	{ LSTRKEY( "update"      ),	  LFUNCVAL( lneopixel_update        ) },
    { LSTRKEY( "__metatable" ),	  LROVAL  ( lneopixel_inst_map      ) },
	{ LSTRKEY( "__index"     ),   LROVAL  ( lneopixel_inst_map      ) },
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_neopixel( lua_State *L ) {
    luaL_newmetarotable(L,"neopixel.inst", (void *)lneopixel_inst_map);
    return 0;
}

MODULE_REGISTER_MAPPED(NEOPIXEL, neopixel, lneopixel_map, luaopen_neopixel);

#endif

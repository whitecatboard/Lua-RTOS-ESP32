/*
 * Lua RTOS, Lua NEOPIXEL module
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

extern LUA_REG_TYPE neopixel_error_map[];

static int lneopixel_setup( lua_State* L ) {
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
	{ LSTRKEY( "error"   ),      LROVAL   ( neopixel_error_map ) },
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

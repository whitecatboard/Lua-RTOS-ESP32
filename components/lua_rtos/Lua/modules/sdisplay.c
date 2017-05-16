/*
 * Lua RTOS, Lua Segment Display module
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

#if CONFIG_LUA_RTOS_LUA_USE_SDISPLAY

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"
#include "sdisplay.h"
#include "modules.h"

#include <stdio.h>
#include <string.h>

#include <drivers/tm1637.h>

// This variables are defined at linker time
extern LUA_REG_TYPE tm1637_error_map[];

static int lsdisplay_attach( lua_State* L ) {
	driver_error_t *error;

    const char *type = luaL_checkstring(L, 1);
    if (strcmp(type,"TM1637") != 0) {
    	luaL_error(L, "type is not supported");
    }

    int8_t scl = luaL_checkinteger(L, 2);
    int8_t sda = luaL_checkinteger(L, 3);
    int8_t segments = luaL_optinteger(L, 4, 4);
    int8_t brightness = luaL_optinteger(L, 5, TM1637_BRIGHT_TYPICAL);

    UNUSED(segments);

    if ((brightness < 0) || (brightness > 7)) {
    	luaL_error(L, "invalid brightness");
    }

	// Create user data
	sdisplay_userdata *udata = (sdisplay_userdata *)lua_newuserdata(L, sizeof(sdisplay_userdata));
    if (!udata) {
    	return 0;
    }

    if ((error = tm1637_setup(scl, sda, &udata->device))) {
    	return luaL_driver_error(L, error);
    }

    udata->brightness = brightness;

    luaL_getmetatable(L, "sdisplay.ins");
    lua_setmetatable(L, -2);

    return 1;
}

static int lsdisplay_write( lua_State* L ) {
	driver_error_t *error;
	sdisplay_userdata *udata;

	// Get user data
	udata = (sdisplay_userdata *)luaL_checkudata(L, 1, "sdisplay.ins");
	luaL_argcheck(L, udata, 1, "segment display expected");

    const char *value = luaL_checkstring(L, 2);

    if ((error = tm1637_write(udata->device, value, udata->brightness))) {
    	return luaL_driver_error(L, error);
    }

	return 0;
}

static int lsdisplay_clear( lua_State* L ) {
	driver_error_t *error;
	sdisplay_userdata *udata;

	// Get user data
	udata = (sdisplay_userdata *)luaL_checkudata(L, 1, "sdisplay.ins");
	luaL_argcheck(L, udata, 1, "segment display expected");

    if ((error = tm1637_clear(udata->device))) {
    	return luaL_driver_error(L, error);
    }

	return 0;
}

static int lsdisplay_set_brightness( lua_State* L ) {
	sdisplay_userdata *udata;

	// Get user data
	udata = (sdisplay_userdata *)luaL_checkudata(L, 1, "sdisplay.ins");
	luaL_argcheck(L, udata, 1, "segment display expected");

	int8_t brightness = luaL_checkinteger(L, 2);

    if ((brightness < 0) || (brightness > 7)) {
    	luaL_error(L, "invalid brightness");
    }

	udata->brightness = brightness;

	return 0;
}

// Destructor
static int lsdisplay_ins_gc (lua_State *L) {
    sdisplay_userdata *udata = NULL;

    udata = (sdisplay_userdata *)luaL_checkudata(L, 1, "sdisplay.ins");
	if (udata) {
	}

	return 0;
}

static const LUA_REG_TYPE lsdisplay_map[] = {
    { LSTRKEY( "attach"  ),			LFUNCVAL( lsdisplay_attach ) },
    { LSTRKEY( "setup"   ),			LFUNCVAL( lsdisplay_attach ) },
	{ LSTRKEY( "error"   ), 		LROVAL  ( tm1637_error_map ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lsdisplay_ins_map[] = {
    { LSTRKEY( "setbrightness"),	LFUNCVAL( lsdisplay_set_brightness ) },
    { LSTRKEY( "setBrightness"),	LFUNCVAL( lsdisplay_set_brightness ) },
    { LSTRKEY( "write"        ),	LFUNCVAL( lsdisplay_write   	   ) },
    { LSTRKEY( "clear"        ),	LFUNCVAL( lsdisplay_clear   	   ) },
	{ LSTRKEY( "__metatable"  ),    LROVAL  ( lsdisplay_ins_map 	   ) },
	{ LSTRKEY( "__index"      ),   	LROVAL  ( lsdisplay_ins_map 	   ) },
	{ LSTRKEY( "__gc"         ),   	LROVAL  ( lsdisplay_ins_gc 		   ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_sdisplay( lua_State *L ) {
    luaL_newmetarotable(L,"sdisplay.ins", (void *)lsdisplay_ins_map);
    return 0;
}

MODULE_REGISTER_MAPPED(SDISPLAY, sdisplay, lsdisplay_map, luaopen_sdisplay);

#endif

/*

thread.start(function()
	display = sdisplay.setup("TM1637", pio.GPIO26, pio.GPIO14)

	time = 0
	while true do
		display:write(time)

		tmr.delay(1)
		time = time + 1
	end
end)

display:setBrightness(7)

*/

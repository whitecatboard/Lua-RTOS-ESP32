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

#include <sdisplay/sdisplay.h>

static int lsdisplay_attach( lua_State* L ) {
	sdisplay_userdata *udata;
	driver_error_t *error;

	// Get display chipset
    int chipset = luaL_checkinteger(L, 1);

    // Get display type from chipset
    sdisplay_type_t type = sdisplay_type(chipset);
    if (type != SDisplayNoType) {
    	if (type == SDisplayTwoWire) {
    		// CLK / DIO
    	    int clk = luaL_checkinteger(L, 2);
    	    int dio = luaL_checkinteger(L, 3);
    	    uint8_t digits = luaL_optinteger(L, 4, 4);

    	    // Create user data
    	    udata = (sdisplay_userdata *)lua_newuserdata(L, sizeof(sdisplay_userdata));
    	    if (!udata) {
    	    	return 0;
    	    }

    	    if ((error = sdisplay_setup(chipset, &udata->device, digits, clk, dio))) {
    	    	return luaL_driver_error(L, error);
    	    }
    	} else if (type == SDisplayI2C) {
    	    uint8_t digits = luaL_optinteger(L, 2, 6);
    		int addr = luaL_optinteger(L, 3, 0);

    	    // Create user data
    	    udata = (sdisplay_userdata *)lua_newuserdata(L, sizeof(sdisplay_userdata));
    	    if (!udata) {
    	    	return 0;
    	    }

    	    if ((error = sdisplay_setup(chipset, &udata->device, digits, addr))) {
    	    	return luaL_driver_error(L, error);
    	    }
    	}
    }

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

    if ((error = sdisplay_write(&udata->device, value))) {
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

    if ((error = sdisplay_clear(&udata->device))) {
    	return luaL_driver_error(L, error);
    }

	return 0;
}

static int lsdisplay_set_brightness( lua_State* L ) {
	driver_error_t *error;
	sdisplay_userdata *udata;

	// Get user data
	udata = (sdisplay_userdata *)luaL_checkudata(L, 1, "sdisplay.ins");
	luaL_argcheck(L, udata, 1, "segment display expected");

	int8_t brightness = luaL_checkinteger(L, 2);

    if ((error = sdisplay_brightness(&udata->device, brightness))) {
    	return luaL_driver_error(L, error);
    }

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
	DRIVER_REGISTER_LUA_ERRORS(sdisplay)

	{ LSTRKEY( "TM1637" ),         LINTVAL( CHIPSET_TM1637     ) },
	{ LSTRKEY( "HT16K3" ),         LINTVAL( CHIPSET_HT16K3     ) },
{ LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lsdisplay_ins_map[] = {
    { LSTRKEY( "setbrightness"),	LFUNCVAL( lsdisplay_set_brightness ) },
    { LSTRKEY( "setBrightness"),	LFUNCVAL( lsdisplay_set_brightness ) },
    { LSTRKEY( "write"        ),	LFUNCVAL( lsdisplay_write   	   ) },
    { LSTRKEY( "clear"        ),	LFUNCVAL( lsdisplay_clear   	   ) },
	{ LSTRKEY( "__metatable"  ),    LROVAL  ( lsdisplay_ins_map 	   ) },
	{ LSTRKEY( "__index"      ),   	LROVAL  ( lsdisplay_ins_map 	   ) },
	{ LSTRKEY( "__gc"         ),   	LFUNCVAL( lsdisplay_ins_gc 		   ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_sdisplay( lua_State *L ) {
    luaL_newmetarotable(L,"sdisplay.ins", (void *)lsdisplay_ins_map);
    return 0;
}

MODULE_REGISTER_MAPPED(SDISPLAY, sdisplay, lsdisplay_map, luaopen_sdisplay);

#endif

/*

display = sdisplay.attach(sdisplay.TM1637, pio.GPIO26, pio.GPIO14)
display:setBrightness(7)

thread.start(function()
	time = 0
	while true do
		display:write(time)

		tmr.delay(1)
		time = time + 1
	end
end)


display = sdisplay.attach(sdisplay.HT16K3)
display:setBrightness(7)

thread.start(function()
	time = 0
	while true do
		display:write(time)

		tmr.delay(1)
		time = time + 1
	end
end)

*/

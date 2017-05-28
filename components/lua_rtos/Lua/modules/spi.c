/*
 * Lua RTOS, SPI wrapper
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
 * and fitness.  In no spi shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SPI

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"
#include "spi.h"
#include "modules.h"

#include <drivers/spi.h>

// This variables are defined at linker time
extern LUA_REG_TYPE spi_error_map[];

static int lspi_setup(lua_State* L) {
	int id, data_bits, is_master, cs;
	driver_error_t *error;
	uint32_t clock;
	int spi_mode = 0;
	int flags = SPI_FLAG_WRITE | SPI_FLAG_READ;

	id = luaL_checkinteger(L, 1);
	is_master = luaL_checkinteger(L, 2) == 1;
	cs = luaL_checkinteger(L, 3);
	clock = luaL_checkinteger(L, 4);
	data_bits = luaL_checkinteger(L, 5);
	spi_mode = luaL_checkinteger(L, 6);

	if (lua_gettop(L) == 7) {
		flags = luaL_checkinteger(L, 7);
	}

	spi_userdata *spi = (spi_userdata *)lua_newuserdata(L, sizeof(spi_userdata));

	if ((error = spi_setup(id, is_master, cs, spi_mode, clock * 1000, flags, &spi->spi_device))) {
	    return luaL_driver_error(L, error);
	}

    luaL_getmetatable(L, "spi.ins");
    lua_setmetatable(L, -2);

	(void)data_bits;
	return 1;
}

static int lspi_select(lua_State* L) {
	driver_error_t *error;
    spi_userdata *spi = NULL;

    spi = (spi_userdata *)luaL_checkudata(L, 1, "spi.ins");
    luaL_argcheck(L, spi, 1, "spi expected");

    if ((error = spi_select(spi->spi_device))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int lspi_deselect(lua_State*L ) {
	driver_error_t *error;
    spi_userdata *spi = NULL;

    spi = (spi_userdata *)luaL_checkudata(L, 1, "spi.ins");
    luaL_argcheck(L, spi, 1, "spi expected");

    if ((error = spi_deselect(spi->spi_device))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int lspi_rw_helper( lua_State *L, int withread ) {
	driver_error_t *error;
	unsigned char value;
	const char *sval;
	spi_userdata *spi = NULL;

	int total = lua_gettop(L), i, j;

	spi = (spi_userdata *)luaL_checkudata(L, 1, "spi.ins");
	luaL_argcheck(L, spi, 1, "spi expected");

	size_t len, residx = 0;

	if (withread)
		lua_newtable(L);

	for (i = 2; i <= total; i++) {
		if(lua_isnumber(L, i)) {
			error = spi_transfer(spi->spi_device, lua_tointeger(L, i), &value);
			if (error) {
				return luaL_driver_error(L, error);
			}

			if(withread) {
				lua_pushinteger(L, value);
				lua_rawseti(L, -2, residx++);
			}
		}
		else if(lua_isstring( L, i )) {
			sval = lua_tolstring(L, i, &len);
			for(j = 0; j < len; j ++) {
				error = spi_transfer(spi->spi_device, sval[j], &value);
				if (error) {
					return luaL_driver_error(L, error);
				}
				if (withread) {
					lua_pushinteger(L, value);
					lua_rawseti(L, -2, residx++);
				}
			}
		}
	}

	return withread ? 1 : 0;
}

static int lspi_write(lua_State* L) {
	return lspi_rw_helper(L, 0);
}

static int lspi_readwrite(lua_State* L) {
	return lspi_rw_helper(L, 1);
}

// Destructor
static int lspi_ins_gc (lua_State *L) {
    spi_userdata *udata = NULL;
    udata = (spi_userdata *)luaL_checkudata(L, 1, "spi.ins");
	if (udata) {
	//	free(udata->instance);
	}

	return 0;
}

static const LUA_REG_TYPE lspi_map[] = {
	{ LSTRKEY( "setup"      ),	 LFUNCVAL( lspi_setup    ) },
	{ LSTRKEY( "error"      ),   LROVAL  ( spi_error_map ) },
	{ LSTRKEY( "WRITE"      ),	 LINTVAL ( SPI_FLAG_WRITE) },
	{ LSTRKEY( "READ"       ),	 LINTVAL ( SPI_FLAG_READ ) },
	{ LSTRKEY( "MASTER"     ),	 LINTVAL ( 1 ) },
	{ LSTRKEY( "SLAVE"      ),	 LINTVAL ( 0 ) },
	SPI_SPI0
	SPI_SPI1
	SPI_SPI2
	SPI_SPI3
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lspi_ins_map[] = {
	{ LSTRKEY( "select"      ),	 LFUNCVAL( lspi_select    ) },
	{ LSTRKEY( "deselect"    ),	 LFUNCVAL( lspi_deselect  ) },
	{ LSTRKEY( "write"       ),	 LFUNCVAL( lspi_write     ) },
	{ LSTRKEY( "readwrite"   ),	 LFUNCVAL( lspi_readwrite ) },
    { LSTRKEY( "__metatable" ),	 LROVAL  ( lspi_ins_map   ) },
	{ LSTRKEY( "__index"     ),  LROVAL  ( lspi_ins_map   ) },
	{ LSTRKEY( "__gc"        ),  LROVAL  ( lspi_ins_gc    ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_spi( lua_State *L ) {
    luaL_newmetarotable(L,"spi.ins", (void *)lspi_ins_map);
    return 0;
}

MODULE_REGISTER_MAPPED(SPI, spi, lspi_map, luaopen_spi);

#endif

/*

 mcp3208 = spi.setup(spi.SPI2, spi.MASTER, pio.GPIO15, 1000, 1, 1, 8)

 while true do
 mcp3208:select()
 mcp3208:deselect()
 end
 */

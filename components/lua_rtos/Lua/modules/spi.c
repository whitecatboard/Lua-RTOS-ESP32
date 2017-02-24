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

#if LUA_USE_SPI

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

	id = luaL_checkinteger(L, 1);
	is_master = luaL_checkinteger(L, 2) == 1;
	cs = luaL_checkinteger(L, 3);
	clock = luaL_checkinteger(L, 4);
	data_bits = luaL_checkinteger(L, 5);
	spi_mode = luaL_checkinteger(L, 6);

	spi_userdata *spi = (spi_userdata *)lua_newuserdata(L, sizeof(spi_userdata));

	spi->spi = id;
	spi->cs = cs;
	spi->speed = clock;
	spi->mode = spi_mode;
	spi->bits = data_bits;

    if ((error = spi_init(spi->spi, is_master))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = spi_set_mode(spi->spi, spi->mode))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = spi_set_speed(spi->spi, spi->speed))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = spi_set_cspin(spi->spi, spi->cs))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = spi_deselect(spi->spi))) {
    	return luaL_driver_error(L, error);
    }

    luaL_getmetatable(L, "spi");
    lua_setmetatable(L, -2);

	return 1;
}

static int lspi_select(lua_State* L) {
	driver_error_t *error;
    spi_userdata *spi = NULL;

    spi = (spi_userdata *)luaL_checkudata(L, 1, "spi");
    luaL_argcheck(L, spi, 1, "spi expected");

    if ((error = spi_set_mode(spi->spi, spi->mode))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = spi_set_speed(spi->spi, spi->speed))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = spi_set_cspin(spi->spi, spi->cs))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = spi_select(spi->spi))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int lspi_deselect(lua_State*L ) {
	driver_error_t *error;
    spi_userdata *spi = NULL;

    spi = (spi_userdata *)luaL_checkudata(L, 1, "spi");
    luaL_argcheck(L, spi, 1, "spi expected");

    if ((error = spi_deselect(spi->spi))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}


static int lspi_rw_helper( lua_State *L, int withread ) {
	unsigned char value;
	const char *sval;
	spi_userdata *spi = NULL;

	int total = lua_gettop(L), i, j;

	spi = (spi_userdata *)luaL_checkudata(L, 1, "spi");
	luaL_argcheck(L, spi, 1, "spi expected");

	size_t len, residx = 0;

	if (withread)
		lua_newtable(L);

	for (i = 2; i <= total; i++) {
		if(lua_isnumber(L, i)) {
			spi_transfer(spi->spi, lua_tointeger(L, i), &value);
			if(withread) {
				lua_pushinteger(L, value);
				lua_rawseti(L, -2, residx++);
			}
		}
		else if(lua_isstring( L, i )) {
			sval = lua_tolstring(L, i, &len);
			for(j = 0; j < len; j ++) {
				spi_transfer(spi->spi, sval[j], &value);
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

    udata = (spi_userdata *)luaL_checkudata(L, 1, "spi");
	if (udata) {
	//	free(udata->instance);
	}

	return 0;
}

static int lspi_index(lua_State *L);
static int lspi_ins_index(lua_State *L);

static const LUA_REG_TYPE lspi_map[] = {
	{ LSTRKEY( "setup"     ),	 LFUNCVAL( lspi_setup     ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lspi_ins_map[] = {
	{ LSTRKEY( "select"    ),	 LFUNCVAL( lspi_select    ) },
	{ LSTRKEY( "deselect"  ),	 LFUNCVAL( lspi_deselect  ) },
	{ LSTRKEY( "write"     ),	 LFUNCVAL( lspi_write     ) },
	{ LSTRKEY( "readwrite" ),	 LFUNCVAL( lspi_readwrite ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lspi_constants_map[] = {
	{ LSTRKEY("error"       ),   LROVAL( spi_error_map )},
	{ LSTRKEY( "MASTER"     ),	 LINTVAL( 1 ) },
	{ LSTRKEY( "SLAVE"      ),	 LINTVAL( 0 ) },
	SPI_SPI0
	SPI_SPI1
	SPI_SPI2
	SPI_SPI3
	{ LNILKEY, LNILVAL }
};

static const luaL_Reg lspi_func[] = {
    { "__index", 	lspi_index },
    { NULL, NULL }
};

static const luaL_Reg lspi_ins_func[] = {
	{ "__gc"   , 	lspi_ins_gc },
    { "__index", 	lspi_ins_index },
    { NULL, NULL }
};

static int lspi_index(lua_State *L) {
	return luaR_index(L, lspi_map, lspi_constants_map);
}

static int lspi_ins_index(lua_State *L) {
	return luaR_index(L, lspi_ins_map, NULL);
}

LUALIB_API int luaopen_spi( lua_State *L ) {
	luaL_newlib(L, lspi_func);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);

    luaL_newmetatable(L, "spi");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    luaL_setfuncs(L, lspi_ins_func, 0);
    lua_pop(L, 1);

    return 1;
}

MODULE_REGISTER_UNMAPPED(SPI, spi, luaopen_spi);

#endif

/*

 mcp3208 = spi.setup(spi.SPI2, spi.MASTER, pio.GPIO15, 1000, 1, 1, 8)
 mcp3208:select()
 mcp3208:deselect()
 */

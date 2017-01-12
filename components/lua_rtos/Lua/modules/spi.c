/*
 * Lua RTOS, SPI wrapper
 *
 * Copyright (C) 2015 - 2016
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

#if LUA_USE_SPI

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"
#include "spi.h"

#include <drivers/spi.h>
#include <drivers/cpu.h>
#include <drivers/gpio.h>

static int lspi_exists( int id ) {
    return ((id >= CPU_FIRST_SPI) && (id <= CPU_LAST_SPI));
}

static int lspi_pins( lua_State* L ) {
    unsigned char sdi, sdo, sck, cs;
    int i;

    for(i=CPU_FIRST_SPI;i<=CPU_LAST_SPI;i++) {
        spi_pins(i, &sdi, &sdo, &sck, &cs);

        printf(
            "spi%d: sdi=%s%02d\t(pin %02d)\tsdo=%s%02d\t(pin %02d)\tsck=%s%02d\t(pin %02d)\n", i,
            gpio_portname(sdi), gpio_name(sdi),cpu_pin_number(sdi),
            gpio_portname(sdo), gpio_name(sdo),cpu_pin_number(sdo),
            gpio_portname(sck), gpio_name(sck),cpu_pin_number(sck)
        );
    }

    return 0;
}

static int lspi_select(lua_State* L) {
    spi_userdata *spi = NULL;

    spi = (spi_userdata *)luaL_checkudata(L, 1, "spi");
    luaL_argcheck(L, spi, 1, "spi expected");

    if (!lspi_exists(spi->spi)) {
        return luaL_error(L, "SPI%d does not exist", spi->spi);
    }

    spi_set_mode(spi->spi, spi->mode);
    spi_set_speed(spi->spi, spi->speed);
    spi_set_cspin(spi->spi, spi->cs);
    spi_deselect(spi->spi);

    return 0;
}

static int lspi_deselect(lua_State*L ) {
    spi_userdata *spi = NULL;

    spi = (spi_userdata *)luaL_checkudata(L, 1, "spi");
    luaL_argcheck(L, spi, 1, "spi expected");

    if (!lspi_exists(spi->spi)) {
        return luaL_error(L, "SPI%d does not exist", spi->spi);
    }

	spi_deselect(spi->spi);

    return 0;
}

static int lspi_setup(lua_State* L) {
	int id, data_bits, is_master, cs;
	driver_error_t *error;
	uint32_t clock;
	int spi_mode = 0;

	id = luaL_checkinteger(L, 1);

    if (!lspi_exists(id)) {
        return luaL_error(L, "SPI%d does not exist", id);
    }

	is_master = luaL_checkinteger(L, 2) == 1;
	if (!is_master)
		return luaL_error(L, "invalid type (only master is supported)");

	cs = luaL_checkinteger(L, 3);
	clock = luaL_checkinteger(L, 4);
	data_bits = luaL_checkinteger(L, 5);
	spi_mode = luaL_checkinteger(L, 6);

	if ((spi_mode < 0) || (spi_mode > 3)) {
		return luaL_error(L, "invalid mode number");
	}

	spi_userdata *spi = (spi_userdata *)lua_newuserdata(L, sizeof(spi_userdata));

	spi->spi = id;
	spi->cs = cs;
	spi->speed = clock;
	spi->mode = spi_mode;
	spi->bits = data_bits;

    if ((error = spi_init(spi->spi))) {
    	return luaL_driver_error(L, error);
    }

    spi_set_mode(spi->spi, spi->mode);
    spi_set_speed(spi->spi, spi->speed);
    spi_set_cspin(spi->spi, spi->cs);
    spi_deselect(spi->spi);

	lua_setmetatable(L, -2);

	return 1;
}

static int lspi_rw_helper( lua_State *L, int withread ) {
	spi_data_type value;
	const char *sval;
	spi_userdata *spi = NULL;

	int total = lua_gettop(L), i, j, id;

	spi = (spi_userdata *)luaL_checkudata(L, 1, "spi");
	luaL_argcheck(L, spi, 1, "spi expected");
  
	size_t len, residx = 0;
  
	id = spi->spi;
	if (!lspi_exists(spi->spi)) {
		return luaL_error(L, "SPI%d does not exist", id);
	}

	if (withread)
		lua_newtable(L);

	for (i = 2; i <= total; i++) {
		if(lua_isnumber(L, i)) {
			value = spi_transfer(spi->spi, lua_tointeger(L, i));
			if(withread) {
				lua_pushinteger(L, value);
				lua_rawseti(L, -2, residx++);
			}
		}
		else if(lua_isstring( L, i )) {
			sval = lua_tolstring(L, i, &len);
			for(j = 0; j < len; j ++) {
				value = spi_transfer(spi->spi, sval[j]);
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

#include "modules.h"

// Module function map
static const LUA_REG_TYPE spi_method_map[] = {
  { LSTRKEY( "setup"     ),	 LFUNCVAL( lspi_setup     ) },
  { LSTRKEY( "pins"      ),	 LFUNCVAL( lspi_pins      ) },
  { LSTRKEY( "select"    ),	 LFUNCVAL( lspi_select    ) },
  { LSTRKEY( "deselect"  ),	 LFUNCVAL( lspi_deselect  ) },
  { LSTRKEY( "write"     ),	 LFUNCVAL( lspi_write     ) },
  { LSTRKEY( "readwrite" ),	 LFUNCVAL( lspi_readwrite ) },
  { LNILKEY, LNILVAL }
};

#if LUA_USE_ROTABLE

static const LUA_REG_TYPE spi_constants_map[] = {
  { LSTRKEY( "MASTER"     ),	 LINTVAL( 1 ) },
  { LSTRKEY( "SLAVE"      ),	 LINTVAL( 0 ) },
  SPI_SPI0
  SPI_SPI1
  SPI_SPI2
  SPI_SPI3
  { LNILKEY, LNILVAL }
};

static int luaL_spi_index(lua_State *L) {
	int res;

	if ((res = luaR_findfunction(L, spi_method_map)) != 0)
		return res;

	const char *key = luaL_checkstring(L, 2);
	const TValue *val = luaR_findentry(spi_constants_map, key, 0, NULL);
	if (val != luaO_nilobject) {
		lua_pushinteger(L, val->value_.i);
		return 1;
	}

	return (int)luaO_nilobject;
}

static const luaL_Reg spi_load_funcs[] = {
    { "__index"    , 	luaL_spi_index },
    { NULL, NULL }
};

static int luaL_mspi_index(lua_State *L) {
  int fres;
  if ((fres = luaR_findfunction(L, spi_method_map)) != 0)
    return fres;

  return (int)luaO_nilobject;
}

static const luaL_Reg mspi_load_funcs[] = {
    { "__index"    , 	luaL_mspi_index },
    { NULL, NULL }
};

#else

static const luaL_Reg spi_map[] = {
	{ NULL, NULL }
};

#endif

LUALIB_API int luaopen_spi( lua_State *L ) {
#if !LUA_USE_ROTABLE
    int n;
    luaL_register( L, AUXLIB_SPI, spi_map );
    
    // Set it as its own metatable
    lua_pushvalue( L, -1 );
    lua_setmetatable( L, -2 );

    // create metatable
    luaL_newmetatable(L, "spi");

    // Module constants  
    int i;
    char buff[5];

    for(i=1;i<=NSPI;i++) {
        sprintf(buff,"SPI%d",i);
        MOD_REG_INTEGER( L, buff, i );
    }

    // Add the MASTER and SLAVE constants (for spi.setup)
    MOD_REG_INTEGER( L, "MASTER", 1 );
    MOD_REG_INTEGER( L, "SLAVE", 0 );  
  
    // metatable.__index = metatable
    lua_pushliteral(L, "__index");
    lua_pushvalue(L,-2);
    lua_rawset(L,-3);
    
    // Setup the methods inside metatable
    luaL_register( L, NULL, spi_method_map );
#else
    luaL_newlib(L, spi_load_funcs);  /* new module */
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);

    luaL_newmetatable(L, "spi");  /* create metatable */
    lua_pushvalue(L, -1);  /* push metatable */
    lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */

    luaL_setfuncs(L, mspi_load_funcs, 0);  /* add file methods to new metatable */
    lua_pop(L, 1);  /* pop new metatable */
#endif

    return 1;
}

LIB_INIT(SPI, spi, luaopen_spi);

#endif


// spi = spi.setup(2, 1, pio.GPIO4, 1, 1, 1)

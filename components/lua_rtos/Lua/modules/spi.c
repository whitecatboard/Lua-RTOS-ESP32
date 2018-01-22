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
 * Lua RTOS, Lua SPI module
 *
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

#include <drivers/gpio.h>
#include <drivers/cpu.h>
#include <drivers/spi.h>

extern spi_bus_t spi_bus[CPU_LAST_SPI - CPU_FIRST_SPI + 1];

static int lspi_pins(lua_State* L) {
	uint8_t table = 0;
	uint16_t count = 0, i = 0;
	int unit = CPU_FIRST_SPI;

	// Check if user wants result as a table, or wants result
	// on the console
	if (lua_gettop(L) == 1) {
		luaL_checktype(L, 1, LUA_TBOOLEAN);
		if (lua_toboolean(L, 1)) {
			table = 1;
		}
	}

	if (table){
		lua_createtable(L, count, 0);
	}

	for(unit = CPU_FIRST_SPI; unit <= CPU_LAST_SPI;unit++) {
		if (!table) {
			printf("SPI%d: ", unit);

			printf("miso=");
			if (spi_bus[spi_idx(unit)].miso >= 0) {
				printf("%s%d ", gpio_portname(spi_bus[spi_idx(unit)].miso), gpio_name(spi_bus[spi_idx(unit)].miso));
			} else {
				printf("unused");
			}

			printf("mosi=");
			if (spi_bus[spi_idx(unit)].mosi >= 0) {
				printf("%s%d ", gpio_portname(spi_bus[spi_idx(unit)].mosi), gpio_name(spi_bus[spi_idx(unit)].mosi));
			} else {
				printf("unused");
			}

			printf("clk=");
			if (spi_bus[spi_idx(unit)].clk >= 0) {
				printf("%s%d ", gpio_portname(spi_bus[spi_idx(unit)].clk), gpio_name(spi_bus[spi_idx(unit)].clk));
			} else {
				printf("unused");
			}

			printf("\r\n");
		} else {
			lua_pushinteger(L, i);

			lua_createtable(L, 0, 4);

			lua_pushinteger(L, unit);
	        lua_setfield (L, -2, "id");

	        lua_pushinteger(L, spi_bus[spi_idx(unit)].miso);
			lua_setfield (L, -2, "miso");

			lua_pushinteger(L, spi_bus[spi_idx(unit)].mosi);
	        lua_setfield (L, -2, "mosi");

	        lua_pushinteger(L, spi_bus[spi_idx(unit)].clk);
			lua_setfield (L, -2, "clk");

			 lua_settable(L,-3);
		}

		i++;
	}

	return table;
}

static int lspi_setpins(lua_State* L) {
	driver_error_t *error;

	int id = luaL_checkinteger(L, 1);
	int miso = luaL_checkinteger(L, 2);
	int mosi = luaL_checkinteger(L, 3);
	int clk = luaL_checkinteger(L, 4);

	if ((error = spi_pin_map(id, miso, mosi, clk))) {
	    return luaL_driver_error(L, error);
	}

	return 0;
}

static int lspi_attach(lua_State* L) {
	int id, data_bits, is_master, cs;
	driver_error_t *error;
	uint32_t clock;
	int spi_mode = 0;
	int flags = DRIVER_ALL_FLAGS;

	id = luaL_checkinteger(L, 1);
	is_master = luaL_checkinteger(L, 2) == 1;
	cs = luaL_checkinteger(L, 3);
	clock = luaL_checkinteger(L, 4);
	data_bits = luaL_checkinteger(L, 5);
	spi_mode = luaL_checkinteger(L, 6);
	flags = luaL_optinteger(L, 7, SPI_FLAG_WRITE | SPI_FLAG_READ);

	spi_userdata *spi = (spi_userdata *)lua_newuserdata(L, sizeof(spi_userdata));

	if ((error = spi_setup(id, is_master, cs, spi_mode, clock, flags, &spi->spi_device))) {
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
	{ LSTRKEY( "attach"     ),	 LFUNCVAL( lspi_attach   ) },
	{ LSTRKEY( "pins"       ),	 LFUNCVAL( lspi_pins     ) },
	{ LSTRKEY( "setpins"    ),	 LFUNCVAL( lspi_setpins  ) },
	DRIVER_REGISTER_LUA_ERRORS(spi)
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
	{ LSTRKEY( "__gc"        ),  LFUNCVAL( lspi_ins_gc    ) },
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

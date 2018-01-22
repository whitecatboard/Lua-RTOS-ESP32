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
 * Lua RTOS, Lua I2C module
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_I2C

#include "lua.h"
#include "error.h"
#include "lauxlib.h"
#include "i2c.h"
#include "modules.h"
#include "error.h"

#include <drivers/i2c.h>
#include <drivers/cpu.h>
#include <drivers/gpio.h>

extern i2c_t i2c[CPU_LAST_I2C + 1];

typedef struct {
	int unit;
	int transaction;
} i2c_user_data_t;

static int li2c_pins(lua_State* L) {
	uint8_t table = 0;
	uint16_t count = 0, i = 0;
	int unit = CPU_FIRST_I2C;

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

	for(unit = CPU_FIRST_I2C; unit <= CPU_LAST_I2C;unit++) {
		if (!table) {
			printf("I2C%d: ", unit);

			printf("sda=");
			if (i2c[unit].sda >= 0) {
				printf("%s%d ", gpio_portname(i2c[unit].sda), gpio_name(i2c[unit].sda));
			} else {
				printf("unused");
			}

			printf("scl=");
			if (i2c[unit].scl >= 0) {
				printf("%s%d ", gpio_portname(i2c[unit].scl), gpio_name(i2c[unit].scl));
			} else {
				printf("unused");
			}


			printf("\r\n");
		} else {
			lua_pushinteger(L, i);

			lua_createtable(L, 0, 3);

			lua_pushinteger(L, unit);
	        lua_setfield (L, -2, "id");

	        lua_pushinteger(L, i2c[unit].sda);
			lua_setfield (L, -2, "sda");

			lua_pushinteger(L, i2c[unit].scl);
	        lua_setfield (L, -2, "scl");

			 lua_settable(L,-3);
		}

		i++;
	}

	return table;
}

static int li2c_setpins(lua_State* L) {
	driver_error_t *error;

	int id = luaL_checkinteger(L, 1);
	int sda = luaL_checkinteger(L, 2);
	int scl = luaL_checkinteger(L, 3);

	if ((error = i2c_pin_map(id, sda, scl))) {
	    return luaL_driver_error(L, error);
	}

	return 0;
}


static int li2c_attach( lua_State* L ) {
	int speed = -1;
	driver_error_t *error;
	int i2cdevice;

    int id = luaL_checkinteger(L, 1);
    int mode = luaL_checkinteger(L, 2);

    if (lua_gettop(L) == 3) {
    	speed = luaL_checkinteger(L, 3);
    }

    if ((error = i2c_setup(id, mode, speed, 0, 0, &i2cdevice))) {
    	return luaL_driver_error(L, error);
    }

    // Allocate userdata
    i2c_user_data_t *user_data = (i2c_user_data_t *)lua_newuserdata(L, sizeof(i2c_user_data_t));
    if (!user_data) {
       	return luaL_exception(L, I2C_ERR_NOT_ENOUGH_MEMORY);
    }

    user_data->unit = i2cdevice;
    user_data->transaction = I2C_TRANSACTION_INITIALIZER;

    luaL_getmetatable(L, "i2c.trans");
    lua_setmetatable(L, -2);

    return 1;
}

static int li2c_setspeed( lua_State* L ) {
	driver_error_t *error;
	i2c_user_data_t *user_data;

	// Get user data
	user_data = (i2c_user_data_t *)luaL_checkudata(L, 1, "i2c.trans");
    luaL_argcheck(L, user_data, 1, "i2c transaction expected");

	int speed = luaL_checkinteger(L, 2);

    if ((error = i2c_setspeed(user_data->unit, speed))) {
    	return luaL_driver_error(L, error);
    }

     return 0;
}

static int li2c_start( lua_State* L ) {
	driver_error_t *error;
	i2c_user_data_t *user_data;

	// Get user data
	user_data = (i2c_user_data_t *)luaL_checkudata(L, 1, "i2c.trans");
    luaL_argcheck(L, user_data, 1, "i2c transaction expected");

    if ((error = i2c_start(user_data->unit, &user_data->transaction))) {
    	return luaL_driver_error(L, error);
    }

     return 0;
}

static int li2c_stop( lua_State* L ) {
	driver_error_t *error;
	i2c_user_data_t *user_data;

	// Get user data
	user_data = (i2c_user_data_t *)luaL_checkudata(L, 1, "i2c.trans");
    luaL_argcheck(L, user_data, 1, "i2c transaction expected");

    if ((error = i2c_stop(user_data->unit, &user_data->transaction))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int li2c_address( lua_State* L ) {
	driver_error_t *error;
	i2c_user_data_t *user_data;

	// Get user data
	user_data = (i2c_user_data_t *)luaL_checkudata(L, 1, "i2c.trans");
    luaL_argcheck(L, user_data, 1, "i2c transaction expected");

    int address = luaL_checkinteger(L, 2);
    int read = 0;

	luaL_checktype(L, 3, LUA_TBOOLEAN);
	if (lua_toboolean(L, 3)) {
		read = 1;
	}

	if ((error = i2c_write_address(user_data->unit, &user_data->transaction, address, read))) {
    	return luaL_driver_error(L, error);
    }
    
    return 0;
}

static int li2c_read( lua_State* L ) {
	driver_error_t *error;
	i2c_user_data_t *user_data;
	char data;

	// Get user data
	user_data = (i2c_user_data_t *)luaL_checkudata(L, 1, "i2c.trans");
    luaL_argcheck(L, user_data, 1, "i2c transaction expected");

    if ((error = i2c_read(user_data->unit, &user_data->transaction, &data, 1))) {
    	return luaL_driver_error(L, error);
    }

    // We need to flush because we need to return reaad data now
    if ((error = i2c_flush(user_data->unit, &user_data->transaction, 1))) {
    	return luaL_driver_error(L, error);
    }

    lua_pushinteger(L, (int)data);

    return 1;
}

static int li2c_write(lua_State* L) {
	driver_error_t *error;
	i2c_user_data_t *user_data;
	int total = lua_gettop(L), i, j;
	char data;
	const char *sval;
	size_t len;

	// Get user data
	user_data = (i2c_user_data_t *)luaL_checkudata(L, 1, "i2c.trans");
    luaL_argcheck(L, user_data, 1, "i2c transaction expected");

	for (i = 2; i <= total; i++) {
		if(lua_isnumber(L, i)) {
			data = (char)(luaL_checkinteger(L, i) & 0xff);

		    if ((error = i2c_write(user_data->unit, &user_data->transaction, &data, sizeof(data)))) {
		    	return luaL_driver_error(L, error);
		    }
		}
		else if(lua_isstring( L, i )) {
			sval = lua_tolstring(L, i, &len);
			for(j = 0; j < len; j ++) {
			    if ((error = i2c_write(user_data->unit, &user_data->transaction, (char *)&sval[j], sizeof(sval[j])))) {
			    	return luaL_driver_error(L, error);
			    }
			}
		}
	}

    return 0;
}

// Destructor
static int li2c_trans_gc (lua_State *L) {
	i2c_user_data_t *user_data = NULL;

    user_data = (i2c_user_data_t *)luaL_testudata(L, 1, "i2c.trans");
    if (user_data) {
    }

    return 0;
}

static const LUA_REG_TYPE li2c_map[] = {
    { LSTRKEY( "attach"  ),			LFUNCVAL( li2c_attach  ) },
	{ LSTRKEY( "pins"    ),	 		LFUNCVAL( li2c_pins     ) },
	{ LSTRKEY( "setpins" ),	 		LFUNCVAL( li2c_setpins  ) },
	{ LSTRKEY( "MASTER"  ),			LINTVAL ( I2C_MASTER   ) },
	{ LSTRKEY( "SLAVE"   ),			LINTVAL ( I2C_SLAVE    ) },
	I2C_I2C0
	I2C_I2C1
	DRIVER_REGISTER_LUA_ERRORS(i2c)
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE li2c_trans_map[] = {
	{ LSTRKEY( "start"      ),		LFUNCVAL( li2c_start     ) },
    { LSTRKEY( "address"     ),		LFUNCVAL( li2c_address   ) },
    { LSTRKEY( "read"        ),		LFUNCVAL( li2c_read      ) },
    { LSTRKEY( "write"       ),		LFUNCVAL( li2c_write     ) },
    { LSTRKEY( "setspeed"    ),		LFUNCVAL( li2c_setspeed  ) },
    { LSTRKEY( "stop"        ),		LFUNCVAL( li2c_stop      ) },
    { LSTRKEY( "__metatable" ),  	LROVAL  ( li2c_trans_map ) },
	{ LSTRKEY( "__index"     ),   	LROVAL  ( li2c_trans_map ) },
	{ LSTRKEY( "__gc"        ),   	LFUNCVAL  ( li2c_trans_gc ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_i2c( lua_State *L ) {
    luaL_newmetarotable(L,"i2c.trans", (void *)li2c_trans_map);
    return 0;
}

MODULE_REGISTER_MAPPED(I2C, i2c, li2c_map, luaopen_i2c);

#endif

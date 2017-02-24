/*
 * Lua RTOS, I2C Lua module
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

#if LUA_USE_I2C

#include "lua.h"
#include "error.h"
#include "lauxlib.h"
#include "i2c.h"
#include "modules.h"
#include "error.h"

#include <drivers/i2c.h>
#include <drivers/cpu.h>

extern LUA_REG_TYPE i2c_error_map[];
extern driver_message_t i2c_errors[];

typedef struct {
	int unit;
	int transaction;
} i2c_user_data_t;

static int li2c_setup( lua_State* L ) {
	driver_error_t *error;

    int id = luaL_checkinteger(L, 1);
    int mode = luaL_checkinteger(L, 2);
    int speed = luaL_checkinteger(L, 3);
    int sda = luaL_checkinteger(L, 4);
    int scl = luaL_checkinteger(L, 5);

    if ((error = i2c_setup(id, mode, speed, sda, scl, 0, 0))) {
    	return luaL_driver_error(L, error);
    }

    // Allocate userdata
    i2c_user_data_t *user_data = (i2c_user_data_t *)lua_newuserdata(L, sizeof(i2c_user_data_t));
    if (!user_data) {
       	return luaL_exception(L, I2C_ERR_NOT_ENOUGH_MEMORY);
    }

    user_data->unit = id;
    user_data->transaction = I2C_TRANSACTION_INITIALIZER;

    luaL_getmetatable(L, "i2c");
    lua_setmetatable(L, -2);

    return 1;
}

static int li2c_start( lua_State* L ) {
	driver_error_t *error;
	i2c_user_data_t *user_data;

	// Get user data
	user_data = (i2c_user_data_t *)luaL_checkudata(L, 1, "i2c");
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
	user_data = (i2c_user_data_t *)luaL_checkudata(L, 1, "i2c");
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
	user_data = (i2c_user_data_t *)luaL_checkudata(L, 1, "i2c");
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
	user_data = (i2c_user_data_t *)luaL_checkudata(L, 1, "i2c");
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

	// Get user data
	user_data = (i2c_user_data_t *)luaL_checkudata(L, 1, "i2c");
    luaL_argcheck(L, user_data, 1, "i2c transaction expected");

    char data = (char)(luaL_checkinteger(L, 2) & 0xff);
    
    esp_err_t i2c_master_write_byte(i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en);

    if ((error = i2c_write(user_data->unit, &user_data->transaction, &data, sizeof(data)))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

// Destructor
static int li2c_trans_gc (lua_State *L) {
	i2c_user_data_t *user_data = NULL;

    user_data = (i2c_user_data_t *)luaL_testudata(L, 1, "i2c");
    if (user_data) {
    }

    return 0;
}

static int li2c_index(lua_State *L);
static int li2c_trans_index(lua_State *L);

static const LUA_REG_TYPE li2c_map[] = {
    { LSTRKEY( "setup"   ),			LFUNCVAL( li2c_setup   ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE li2c_trans_map[] = {
	{ LSTRKEY( "start"   ),			LFUNCVAL( li2c_start   ) },
    { LSTRKEY( "address" ),			LFUNCVAL( li2c_address ) },
    { LSTRKEY( "read"    ),			LFUNCVAL( li2c_read    ) },
    { LSTRKEY( "write"   ),			LFUNCVAL( li2c_write   ) },
    { LSTRKEY( "stop"    ),			LFUNCVAL( li2c_stop    ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE li2c_constants_map[] = {
	{ LSTRKEY( "MASTER"  ),			LINTVAL ( I2C_MASTER   ) },
	{ LSTRKEY( "SLAVE"   ),			LINTVAL ( I2C_SLAVE    ) },
	I2C_I2C0
	I2C_I2C1

	// Error definitions
	{LSTRKEY("error"     ),         LROVAL( i2c_error_map )},

	{ LNILKEY, LNILVAL }
};

static const luaL_Reg li2c_func[] = {
    { "__index", 	li2c_index },
    { NULL, NULL }
};

static const luaL_Reg li2c_trans_func[] = {
	{ "__gc"   , 	li2c_trans_gc },
    { "__index", 	li2c_trans_index },
    { NULL, NULL }
};

static int li2c_index(lua_State *L) {
	return luaR_index(L, li2c_map, li2c_constants_map);
}

static int li2c_trans_index(lua_State *L) {
	return luaR_index(L, li2c_trans_map, NULL);
}

LUALIB_API int luaopen_i2c( lua_State *L ) {
    luaL_newlib(L, li2c_func);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);

    luaL_newmetatable(L, "i2c");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    luaL_setfuncs(L, li2c_trans_func, 0);
    lua_pop(L, 1);

    return 1;
}

MODULE_REGISTER_UNMAPPED(I2C, i2c, luaopen_i2c);

#endif

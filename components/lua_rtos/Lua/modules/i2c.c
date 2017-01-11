/*
 * Lua RTOS, I2C Lua module
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

#if LUA_USE_I2C

#include "lua.h"
#include "error.h"
#include "lauxlib.h"
#include "i2c.h"
#include "modules.h"
#include "error.h"

#include <drivers/i2c.h>
#include <drivers/cpu.h>

typedef struct {
	char *data;
} i2c_user_data_t;

static int transactions[CPU_LAST_I2C + 1] = {
		I2C_TRANSACTION_INITIALIZER,
		I2C_TRANSACTION_INITIALIZER
};

extern char *i2c_errors[];

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

    return 0;
}

static int li2c_start( lua_State* L ) {
	driver_error_t *error;
	int transaction = I2C_TRANSACTION_INITIALIZER;

    int id = luaL_checkinteger(L, 1);

    if ((error = i2c_start(id, &transaction))) {
    	return luaL_driver_error(L, error);
    }
    
    transactions[id] = transaction;

    return 0;
}

static int li2c_stop( lua_State* L ) {
	driver_error_t *error;
	int transaction = I2C_TRANSACTION_INITIALIZER;

    int id = luaL_checkinteger(L, 1);

    if (id < CPU_LAST_I2C + 1) {
    	transaction = transactions[id];
    }

    if ((error = i2c_stop(id, &transaction))) {
    	return luaL_driver_error(L, error);
    }

    transactions[id] = transaction;

    return 0;
}

static int li2c_address( lua_State* L ) {
	driver_error_t *error;
	int transaction = I2C_TRANSACTION_INITIALIZER;

	int id = luaL_checkinteger(L, 1);
    int address = luaL_checkinteger(L, 2);
    int read = 0;

	luaL_checktype(L, 3, LUA_TBOOLEAN);
	if (lua_toboolean(L, 3)) {
		read = 1;
	}

    if (id < CPU_LAST_I2C + 1) {
    	transaction = transactions[id];
    }

    if ((error = i2c_write_address(id, &transaction, address, read))) {
    	return luaL_driver_error(L, error);
    }
    
    return 0;
}

static int li2c_read( lua_State* L ) {
	driver_error_t *error;
	int transaction = I2C_TRANSACTION_INITIALIZER;
	char data;

    int id = luaL_checkinteger(L, 1);
    
    if (id < CPU_LAST_I2C + 1) {
    	transaction = transactions[id];
    }

    if ((error = i2c_read(id, &transaction, &data, 1))) {
    	return luaL_driver_error(L, error);
    }

    // We need to flush because we need to return reaad data now
    if ((error = i2c_flush(id, &transaction, 1))) {
    	return luaL_driver_error(L, error);
    }

    lua_pushinteger(L, (int)data);

    return 1;
}

static int li2c_write(lua_State* L) {
	driver_error_t *error;
	int transaction = I2C_TRANSACTION_INITIALIZER;

    int id = luaL_checkinteger(L, 1);
    char data = (char)(luaL_checkinteger(L, 2) & 0xff);
    
    if (id < CPU_LAST_I2C + 1) {
    	transaction = transactions[id];
    }

    if ((error = i2c_write(id, &transaction, &data, sizeof(data)))) {
    	return luaL_driver_error(L, error);
    }

    // We need to flush because data buffers are on the stack and if we
    // flush on the stop condition its values will be indeterminate
    if ((error = i2c_flush(id, &transaction, 1))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static const LUA_REG_TYPE li2c[] = {
    { LSTRKEY( "setup"   ),			LFUNCVAL( li2c_setup   ) },
    { LSTRKEY( "start"   ),			LFUNCVAL( li2c_start   ) },
    { LSTRKEY( "stop"    ),			LFUNCVAL( li2c_stop    ) },
    { LSTRKEY( "address" ),			LFUNCVAL( li2c_address ) },
    { LSTRKEY( "read"    ),			LFUNCVAL( li2c_read    ) },
    { LSTRKEY( "write"   ),			LFUNCVAL( li2c_write   ) },
    { LSTRKEY( "MASTER"  ),			LINTVAL ( I2C_MASTER   ) },
    { LSTRKEY( "SLAVE"   ),			LINTVAL ( I2C_SLAVE    ) },
	I2C_I2C0
	I2C_I2C1
    { LNILKEY, LNILVAL }
};

int luaopen_i2c(lua_State* L) {
	return 0;
}

LUA_OS_MODULE(I2C, i2c, li2c);

#endif

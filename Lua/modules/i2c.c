/*
 * Whitecat, I2C Lua module
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

#include "whitecat.h"

#if LUA_USE_I2C

#include "lua.h"
#include "lauxlib.h"

#include <drivers/cpu/error.h>
#include <drivers/i2c/i2c.h>

static int li2c_setup( lua_State* L ) {
    tdriver_error *error;

    int total = lua_gettop(L);
    int id = luaL_checkinteger(L, 1);
    int speed = luaL_checkinteger(L, 2);
    int sda;
    int scl;

    // Some integrity checks
    if (!platform_i2c_exists(id)) {
        return luaL_error(L, "I2C%d does not exist", id);
    }

    if ((speed <= 0) || (speed > 1000)) {
        return luaL_error(L, "Invalid speed");        
    }
    
    if (id > NI2CHW) {
        // SDA and SCL is needed
        if (total != 4) {
            return luaL_error(L, "Missing SDA / SCL arguments");                    
        }
        
        sda = luaL_checkinteger(L, 3);
        scl = luaL_checkinteger(L, 4);
        
        if (sda == scl) {
            return luaL_error(L, "SDA / SCL must be different");      
        }
    }
    
    // Setup
    if ((error = i2c_setup(id, speed, sda, scl))) {
        return luaL_driver_error(L, "I2C can't setup", error);
    }

    return 0;
}

static int li2c_start( lua_State* L ) {
    int id = luaL_checkinteger(L, 1);

    // Some integrity checks
    if (!platform_i2c_exists(id)) {
        return luaL_error(L, "I2C%d does not exist", id);
    }
    
    i2c_start(id);

    return 0;
}

static int li2c_stop( lua_State* L ) {
    int id = luaL_checkinteger(L, 1);

    // Some integrity checks
    if (!platform_i2c_exists(id)) {
        return luaL_error(L, "I2C%d does not exist", id);
    }

    i2c_stop(id);
    
    return 0;
}

static int li2c_address( lua_State* L ) {
    int id = luaL_checkinteger(L, 1);
    int address = luaL_checkinteger(L, 2);
    int direction = luaL_checkinteger(L, 3);

    // Some integrity checks
    if (!platform_i2c_exists(id)) {
        return luaL_error(L, "I2C%d does not exist", id);
    }

    if (address >= 0b10000000000) {
        return luaL_error(L, "Ivalid address");        
    }
    
    if ((direction != 0) && (direction != 1)) {
        return luaL_error(L, "Ivalid direction");
    }

    lua_pushboolean(L, i2c_write_address(id, address, direction));
    
    return 1;
}

static int li2c_read( lua_State* L ) {
    int id = luaL_checkinteger(L, 1);
    char data;
    
    // Some integrity checks
    if (!platform_i2c_exists(id)) {
        return luaL_error(L, "I2C%d does not exist", id);
    }

    data = i2c_read(id);
    
    lua_pushinteger(L, data & 0x000000ff);
   
    return 1;
}

static int li2c_write(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    int data = luaL_checkinteger(L, 2);
    
    // Some integrity checks
    if (!platform_i2c_exists(id)) {
        return luaL_error(L, "I2C%d does not exist", id);
    }

    lua_pushboolean(L, i2c_write(id, (char)(data & 0x000000ff)));

    return 1;
}

static const luaL_Reg li2c[] = {
    {"setup", li2c_setup},
    {"start", li2c_start},
    {"stop", li2c_stop},
    {"address", li2c_address},
    {"read", li2c_read},
    {"write", li2c_write},
    {NULL, NULL}
};

int luaopen_i2c(lua_State* L) {
    luaL_newlib(L, li2c);

    int i;
    char buff[7];

    for(i=1;i<=NI2CHW;i++) {
        sprintf(buff,"I2C%d",i);
        lua_pushinteger(L, i);
        lua_setfield(L, -2, buff);
    }

    for(i=1;i<=NI2CBB;i++) {
        sprintf(buff,"I2CBB%d",i);
        lua_pushinteger(L, i + NI2CHW);
        lua_setfield(L, -2, buff);
    }

    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "WRITE");
    
    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "READ");

    return 1;
}

#endif
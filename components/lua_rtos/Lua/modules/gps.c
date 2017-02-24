/*
 * Lua RTOS, gps Lua module
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

#if LUA_USE_GPS

#include "lua.h"
#include "lauxlib.h"

#if USE_GPS

#include <drivers/gps/gps.h>

// Module flags
static unsigned int flags = 0;

#define LGPS_STARTED (1 << 0)

static int lgps_start(lua_State* L) {
    if (!(flags & LGPS_STARTED)) {
        platform_gps_start();
        
        flags |= LGPS_STARTED;
    } else {
        return luaL_error(L, "gps yet started");
    }
    
    return 0;
}

static int lgps_stop(lua_State* L) {
    if (flags & LGPS_STARTED) {
        platform_gps_stop();

        flags &= ~LGPS_STARTED;
    } else {
        return luaL_error(L, "gps not started");
    }
    
    return 0;    
}

static int lgps_pos(lua_State* L) {
    struct position pos;
    
    int timeout = luaL_optinteger(L, 1, 0xffffffff);
    
    // Read position
    if (gps_get_pos(&pos, timeout)) {
        // No position available
        lua_pushnil(L);
        return 1;
    }
    
    lua_pushnumber(L, pos.lat);
    lua_pushnumber(L, pos.lon);
    lua_pushinteger(L, pos.sats);
    lua_pushinteger(L, pos.when);

    return 4;
}

static const luaL_Reg gps[] = {
    {"start", lgps_start}, 
    {"stop", lgps_stop}, 
    {"position", lgps_pos}, 
    {NULL, NULL}
};

int luaopen_gps(lua_State* L) {
    luaL_newlib(L, gps);
    
    return 1;
}

#endif

#endif

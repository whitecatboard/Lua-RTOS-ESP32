/*
 * Lua RTOS, Lua event module
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

#if LUA_USE_EVENT

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"
#include "event.h"
#include "modules.h"

// List of events
static struct list event_list;

// This variables are defined at linker time
extern LUA_REG_TYPE event_error_map[];

static int levent_create( lua_State* L ) {
	// Create user data
    event_userdata *udata = (event_userdata *)lua_newuserdata(L, sizeof(event_userdata));
    if (!udata) {
    	// TO DO: exception

    	return 0;
    }

    // Init mutex
    mtx_init(&udata->mtx, NULL, NULL, 0);

    // Create listener list
    list_init(&udata->listener_list, 1);

    luaL_getmetatable(L, "event");
    lua_setmetatable(L, -2);

    return 1;
}

static int levent_addlistener( lua_State* L ) {
    event_userdata *udata = NULL;
    int *func;
    int id;
    int ret;

    // Get user data
    udata = (event_userdata *)luaL_checkudata(L, 1, "event");
    luaL_argcheck(L, udata, 1, "event expected");

    // Check for function reference
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // Allocate space for function reference
    func = (int *)calloc(1,sizeof(int));
    if (!func) {
    	// TO DO: exception
    	return 0;
    }

    *func = luaL_ref(L, LUA_REGISTRYINDEX);

    // Add function reference into the listener list
    mtx_lock(&udata->mtx);

    ret = list_add(&udata->listener_list, func, &id);
    if (ret) {
    	// TO DO: exception
    	mtx_unlock(&udata->mtx);
    	return 0;
    }

    mtx_unlock(&udata->mtx);

    lua_pushinteger(L, id);

    return 1;
}

static int levent_broadcast( lua_State* L ) {
    event_userdata *udata = NULL;
	int idx = 1;
	int *func;
	int status;

    // Get user data
	udata = (event_userdata *)luaL_checkudata(L, 1, "event");
    luaL_argcheck(L, udata, 1, "event expected");

    // Call to all listeners
    mtx_lock(&udata->mtx);

    while (idx >= 1) {
    	// Get listener
        if (list_get(&udata->listener_list, idx, (void **)&func)) {
        	break;
        }

        // There is a listener

        // Call to listener function
        lua_rawgeti(L, LUA_REGISTRYINDEX, *func);
        if ((status = lua_pcall(L, 0, 0, 0)) != LUA_OK) {
        	// Error in call
        }

        // Next listener
        idx = list_next(&udata->listener_list, idx);
    }

    mtx_unlock(&udata->mtx);

    return 0;
}

// Destructor
static int levent_ins_gc (lua_State *L) {
    event_userdata *udata = NULL;

    udata = (event_userdata *)luaL_checkudata(L, 1, "event");
	if (udata) {
	}

	return 0;
}

static int levent_index(lua_State *L);
static int levent_ins_index(lua_State *L);

static const LUA_REG_TYPE levent_map[] = {
    { LSTRKEY( "create"  ),	LFUNCVAL( levent_create ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE levent_ins_map[] = {
	{ LSTRKEY( "addlistener"),	LFUNCVAL( levent_addlistener    ) },
  	{ LSTRKEY( "broadcast"  ),	LFUNCVAL( levent_broadcast 	    ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE levent_constants_map[] = {
	{LSTRKEY("error"), 			 LROVAL( event_error_map )},
	{ LNILKEY, LNILVAL }
};

static const luaL_Reg levent_func[] = {
    { "__index", 	levent_index },
    { NULL, NULL }
};

static const luaL_Reg levent_ins_func[] = {
	{ "__gc"   , 	levent_ins_gc },
    { "__index", 	levent_ins_index },
    { NULL, NULL }
};

static int levent_index(lua_State *L) {
	return luaR_index(L, levent_map, levent_constants_map);
}

static int levent_ins_index(lua_State *L) {
	return luaR_index(L, levent_ins_map, NULL);
}

LUALIB_API int luaopen_event( lua_State *L ) {
	luaL_newlib(L, levent_func);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);

    luaL_newmetatable(L, "event");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    luaL_setfuncs(L, levent_ins_func, 0);
    lua_pop(L, 1);

    return 1;
}

MODULE_REGISTER_UNMAPPED(EVENT, event, luaopen_event);

#endif


/*

e = event.create()

e:addlistener(function()
  print("hi from 1")
end)

e:addlistener(function()
  print("hi from 2")
end)

thread.start(function()
  while true do
	  e:broadcast(false)
	  print("hi from master")
  end
end)

 */

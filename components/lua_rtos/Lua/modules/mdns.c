/*
 * Lua RTOS, Lua MDNS module
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

#if CONFIG_LUA_RTOS_LUA_USE_MDNS
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "modules.h"
#include "error.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <mdns.h>
#include "tcpip_adapter.h"

#include <sys/status.h>

// Module errors
#define LUA_MDNS_ERR_CANT_CREATE_SERVICE (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  0)
#define LUA_MDNS_ERR_CANT_STOP           (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  1)
#define LUA_MDNS_ERR_CANT_XXX            (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  2)

DRIVER_REGISTER_ERROR(MDNS, mdns, CannotCreateService, "can't create service", LUA_MDNS_ERR_CANT_CREATE_SERVICE);
DRIVER_REGISTER_ERROR(MDNS, mdns, CannotStop, "can't stop", LUA_MDNS_ERR_CANT_STOP);
DRIVER_REGISTER_ERROR(MDNS, mdns, CannotXXX, "can't xxx", LUA_MDNS_ERR_CANT_XXX);

typedef struct {
    lua_State *L;
    mdns_server_t *client;
} mdns_userdata;

// Lua: result = setup( id, clock )
static int lmdns_start( lua_State* L ){
    int rc = 0;
    int interface = luaL_checkinteger( L, 1 );
    const char *hostname = 0;
    const char *instance = 0;
		if (lua_gettop(L) > 1) {
	    hostname = luaL_checkstring( L, 2 );
			if (lua_gettop(L) > 2) {
			  instance = luaL_checkstring( L, 3 );
			}
		}

    mdns_userdata *mdns = (mdns_userdata *)lua_newuserdata(L, sizeof(mdns_userdata));
    mdns->L = L;

    rc = mdns_init(interface, &(mdns->client));
    if (rc < 0){
      return luaL_exception(L, LUA_MDNS_ERR_CANT_CREATE_SERVICE);
    }

		if(hostname != 0) {
			rc = mdns_set_hostname(mdns->client, hostname);
		  if (rc < 0){
		    printf( "mdns: could not set hostname\n");
		  }
		}
		
		if(instance != 0) {
			rc = mdns_set_instance(mdns->client, instance);
		  if (rc < 0){
		    printf( "mdns: could not set instance name\n");
		  }
		}
		
    luaL_getmetatable(L, "mdns.cli");
    lua_setmetatable(L, -2);

    return 1;
}

static int lmdns_xxx( lua_State* L ) {
    int rc;
    mdns_userdata *mdns = (mdns_userdata *)luaL_checkudata(L, 1, "mdns.cli");
    luaL_argcheck(L, mdns, 1, "mdns expected");
    
    rc = 0; //mdns_(mdns->client);
    if (rc == 0) {
        return 0;
    } else {
      return luaL_exception(L, LUA_MDNS_ERR_CANT_STOP);
    }

    return 0;
}

static int lmdns_stop( lua_State* L ) {
    int rc;
    mdns_userdata *mdns = (mdns_userdata *)luaL_checkudata(L, 1, "mdns.cli");
    luaL_argcheck(L, mdns, 1, "mdns expected");
    
    rc = mdns_service_remove_all(mdns->client);
    if (rc == 0) {
        return 0;
    } else {
      return luaL_exception(L, LUA_MDNS_ERR_CANT_STOP);
    }

    return 0;
}

// Destructor
static int lmdns_service_gc (lua_State *L) {
    mdns_userdata *mdns = NULL;
    
    mdns = (mdns_userdata *)luaL_testudata(L, 1, "mdns.cli");
    if (mdns) {        
				mdns_free(mdns->client);
    }
   
    return 0;
}

static const LUA_REG_TYPE lmdns_map[] = {
  { LSTRKEY( "start"      ),   LFUNCVAL( lmdns_start     ) },

  { LSTRKEY("WIFI_STA"), LINTVAL(TCPIP_ADAPTER_IF_STA) },
  { LSTRKEY("WIFI_AP"), LINTVAL(TCPIP_ADAPTER_IF_AP) },
  { LSTRKEY("ETH"), LINTVAL(TCPIP_ADAPTER_IF_ETH) },
  { LSTRKEY("SPI_ETH"), LINTVAL(TCPIP_ADAPTER_IF_SPI_ETH) },

  { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lmdns_service_map[] = {
  { LSTRKEY( "stop"  ),         LFUNCVAL( lmdns_stop       ) },
  { LSTRKEY( "xxx"   ),         LFUNCVAL( lmdns_xxx        ) },
  { LSTRKEY( "__metatable" ),   LROVAL  ( lmdns_service_map ) },
  { LSTRKEY( "__index"     ),    LROVAL  ( lmdns_service_map ) },
  { LSTRKEY( "__gc"        ),    LROVAL  ( lmdns_service_gc  ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_mdns( lua_State *L ) {
    luaL_newmetarotable(L,"mdns.cli", (void *)lmdns_service_map);
    return 0;
}

MODULE_REGISTER_MAPPED(MDNS, mdns, lmdns_map, luaopen_mdns);

#endif

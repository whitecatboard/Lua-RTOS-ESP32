/*
 * Copyright (C) 2015 - 2018, Thomas E. Horner (whitecatboard.org@horner.it)
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
 * Lua RTOS, Lua MDNS net module
 *
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

#define DEFAULT_TIMEOUT_SECONDS 2

// Module errors
#define LUA_MDNS_ERR_CANT_CREATE_SERVICE (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  0)
#define LUA_MDNS_ERR_CANT_STOP           (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  1)
#define LUA_MDNS_ERR_CANT_RESOLVE_HOST   (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  2)
#define LUA_MDNS_ERR_CANT_FIND_SERVICE   (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  3)
#define LUA_MDNS_ERR_CANT_ADD_SERVICE    (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  4)
#define LUA_MDNS_ERR_CANT_REMOVE_SERVICE (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  5)

// Register drivers and errors
DRIVER_REGISTER_BEGIN(MDNS,mdns,NULL,NULL,NULL);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotCreateService, "can't create service", LUA_MDNS_ERR_CANT_CREATE_SERVICE);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotStop, "can't stop", LUA_MDNS_ERR_CANT_STOP);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotResolveHost, "can't resolve host", LUA_MDNS_ERR_CANT_RESOLVE_HOST);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotFindService, "can't find service", LUA_MDNS_ERR_CANT_FIND_SERVICE);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotAddService, "can't add service", LUA_MDNS_ERR_CANT_ADD_SERVICE);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotRemoveService, "can't remove service", LUA_MDNS_ERR_CANT_REMOVE_SERVICE);
DRIVER_REGISTER_END(MDNS,mdns,NULL,NULL,NULL);

typedef struct {
    lua_State *L;
    mdns_server_t *client;
} mdns_userdata;

// Lua: result = setup( id, clock )
static int lmdns_start( lua_State* L ){
  int rc;
  int interface = luaL_checkinteger( L, 1 );
  const char *hostname = luaL_optstring( L, 2, NULL );
  const char *instance = luaL_optstring( L, 3, NULL );

  mdns_userdata *mdns = (mdns_userdata *)lua_newuserdata(L, sizeof(mdns_userdata));
  mdns->L = L;

  rc = mdns_init(interface, &(mdns->client));
  if (rc != 0) {
    return luaL_exception(L, LUA_MDNS_ERR_CANT_CREATE_SERVICE);
  }

	if(hostname != 0) {
		rc = mdns_set_hostname(mdns->client, hostname);
		if (rc != 0) {
	    printf( "mdns: could not set hostname\n");
	  }
	}
	
	if(instance != 0) {
		rc = mdns_set_instance(mdns->client, instance);
		if (rc != 0) {
	    printf( "mdns: could not set instance name\n");
	  }
	}
	
  luaL_getmetatable(L, "mdns.cli");
  lua_setmetatable(L, -2);

  return 1;
}

static int results_to_table( lua_State* L, mdns_userdata *mdns )
{
	char tmp[46];
	size_t count = mdns_result_get_count(mdns->client);

  lua_newtable(L);
  for(size_t i=0; i<count; i++) {
    const mdns_result_t * res = mdns_result_get(mdns->client, i);
    if (res) {
  
			/*
      printf("mdns %u: " IPSTR " " IPV6STR "\n", i+1, 
             IP2STR(&res->addr), IPV62STR(res->addrv6));

      printf("mdns %u: %s \"%s\" " IPSTR " " IPV6STR " %u %s\n", i+1, 
			       (res->host)?res->host:"", (res->instance)?res->instance:"", 
			       IP2STR(&res->addr), IPV62STR(res->addrv6),
			       res->port, (res->txt)?res->txt:"");
			*/

			//put row into table
			
			lua_newtable(L);
			lua_pushnumber(L, i+1);
			lua_pushvalue(L, -2);
			lua_settable(L, -4);

			if(res->host) {
				lua_pushstring(L, "hostname");
				lua_pushstring(L, (res->host)?res->host:"");
				lua_settable(L, -3);
			}

			if(res->instance) {
				lua_pushstring(L, "instance");
				lua_pushstring(L, (res->instance)?res->instance:"");
				lua_settable(L, -3);
			}

			if(!ip4_addr_isany(&res->addr)) {
				lua_pushstring(L, "ip");
				snprintf(tmp, sizeof(tmp), IPSTR, ip4_addr1_16(&res->addr),ip4_addr2_16(&res->addr),ip4_addr3_16(&res->addr),ip4_addr4_16(&res->addr));
				lua_pushstring(L, tmp);
				lua_settable(L, -3);
			}

			if(!ip6_addr_isany(&res->addrv6)) {
				lua_pushstring(L, "ipv6");
				snprintf(tmp, sizeof(tmp), IPV6STR, IPV62STR(res->addrv6));
				lua_pushstring(L, tmp);
				lua_settable(L, -3);
			}

			if(res->port) {
				lua_pushstring(L, "port");
				lua_pushinteger(L, res->port);
				lua_settable(L, -3);
			}

			if(res->txt) {
				lua_pushstring(L, "txt");
				lua_pushstring(L, (res->txt)?res->txt:"");
				lua_settable(L, -3);
			}

			lua_pop(L, 1);
										 
    }
  }
  mdns_result_free(mdns->client);
	return 1; //one table
}

static int lmdns_resolve_host( lua_State* L ) {
  int rc;
  mdns_userdata *mdns = (mdns_userdata *)luaL_checkudata(L, 1, "mdns.cli");
  const char *hostname = luaL_checkstring( L, 2 );
  int seconds = luaL_optinteger(L, 3, DEFAULT_TIMEOUT_SECONDS);
	if(seconds == 0) seconds = DEFAULT_TIMEOUT_SECONDS;

  luaL_argcheck(L, mdns, 1, "mdns expected");

  rc = mdns_query(mdns->client, hostname, 0, 1000*seconds);
  if (rc) {
 		return results_to_table(L, mdns);
  }

  return luaL_exception(L, LUA_MDNS_ERR_CANT_RESOLVE_HOST);
}

static int lmdns_find_service( lua_State* L ) {
  int rc;
  mdns_userdata *mdns = (mdns_userdata *)luaL_checkudata(L, 1, "mdns.cli");
  const char *service = luaL_checkstring( L, 2 );
  const char *protocol = luaL_checkstring( L, 3 );
  int seconds = luaL_optinteger(L, 4, DEFAULT_TIMEOUT_SECONDS);
	if(seconds == 0) seconds = DEFAULT_TIMEOUT_SECONDS;
	
  luaL_argcheck(L, mdns, 1, "mdns expected");

  rc = mdns_query(mdns->client, service, protocol, 1000*seconds);
  if (rc) {
  	return results_to_table(L, mdns);
  }

  return luaL_exception(L, LUA_MDNS_ERR_CANT_FIND_SERVICE);
}

static int lmdns_add_service( lua_State* L ) {
  int rc;
  mdns_userdata *mdns = (mdns_userdata *)luaL_checkudata(L, 1, "mdns.cli");
  const char *service = luaL_checkstring( L, 2 );
  const char *protocol = luaL_checkstring( L, 3 );
  int port = luaL_checkinteger( L, 4 );
  const char *instance = 0;
  size_t txt_items = 0;
	const char ** txt_data = 0;
  
	if (lua_gettop(L) > 4) {
	  instance = luaL_checkstring( L, 5 );

		if (lua_gettop(L) > 5 && lua_istable(L, 6)) {

			txt_items = lua_rawlen(L, 6); // size of the txt table
			txt_data = (const char **)malloc(sizeof(char *) * txt_items);
			for(int i=0; i<txt_items; i++) {
				lua_rawgeti(L, 6, i+1);
				txt_data[i] = lua_tostring(L, -1);
				lua_pop(L,1);
			}
		}
	}

  luaL_argcheck(L, mdns, 1, "mdns expected");

  rc = mdns_service_add(mdns->client, service, protocol, port);
  if (rc != 0) {
    return luaL_exception(L, LUA_MDNS_ERR_CANT_ADD_SERVICE);
  }

	if(instance != 0) {
	  rc = mdns_service_instance_set(mdns->client, service, protocol, instance);
		if (rc != 0) {
	    printf( "mdns: could not set instance name\n");
	  }
	}

	if (txt_items>0) {
		mdns_service_txt_set(mdns->client, service, protocol, txt_items, txt_data);
		free(txt_data);
		txt_data = 0;
	}

  return 0;
}

static int lmdns_remove_service( lua_State* L ) {
  int rc;
  mdns_userdata *mdns = (mdns_userdata *)luaL_checkudata(L, 1, "mdns.cli");
  const char *service = luaL_checkstring( L, 2 );
  const char *protocol = luaL_checkstring( L, 3 );

  luaL_argcheck(L, mdns, 1, "mdns expected");

  rc = mdns_service_remove(mdns->client, service, protocol);
  if (rc != 0) {
    return luaL_exception(L, LUA_MDNS_ERR_CANT_REMOVE_SERVICE);
  }
  return 0;
}

static int lmdns_stop( lua_State* L ) {
  int rc;
  mdns_userdata *mdns = (mdns_userdata *)luaL_checkudata(L, 1, "mdns.cli");
  luaL_argcheck(L, mdns, 1, "mdns expected");
  
  rc = mdns_service_remove_all(mdns->client);
  if (rc != 0) {
    return luaL_exception(L, LUA_MDNS_ERR_CANT_STOP);
  }
  return 0;
}

// Destructor
static int lmdns_service_gc (lua_State *L) {
  mdns_userdata *mdns = NULL;

  mdns = (mdns_userdata *)luaL_testudata(L, 1, "mdns.cli");
  if (mdns) {        
    mdns_service_remove_all(mdns->client);
		mdns_free(mdns->client);
  }
 
  return 0;
}

static const LUA_REG_TYPE mdns_map[] = {
  { LSTRKEY( "start"      ),   LFUNCVAL( lmdns_start     ) },

  { LSTRKEY( "WIFI_STA" ),     LINTVAL( TCPIP_ADAPTER_IF_STA     ) },
  { LSTRKEY( "WIFI_AP"  ),     LINTVAL( TCPIP_ADAPTER_IF_AP      ) },
  { LSTRKEY( "ETH"      ),     LINTVAL( TCPIP_ADAPTER_IF_ETH     ) },
#if CONFIG_LUA_RTOS_LUA_USE_NET && CONFIG_LUA_RTOS_ETH_HW_TYPE_SPI
  { LSTRKEY( "SPI_ETH"  ),     LINTVAL( TCPIP_ADAPTER_IF_SPI_ETH ) },
#endif

  // Error definitions
  DRIVER_REGISTER_LUA_ERRORS(mdns)
  { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lmdns_service_map[] = {
  { LSTRKEY( "stop"  ),         LFUNCVAL( lmdns_stop             ) },
  { LSTRKEY( "resolvehost"   ), LFUNCVAL( lmdns_resolve_host     ) },
  { LSTRKEY( "findservice"   ), LFUNCVAL( lmdns_find_service     ) },
  { LSTRKEY( "addservice"    ), LFUNCVAL( lmdns_add_service      ) },
  { LSTRKEY( "removeservice" ), LFUNCVAL( lmdns_remove_service   ) },
  { LSTRKEY( "__metatable" ),   LROVAL  ( lmdns_service_map      ) },
  { LSTRKEY( "__index"     ),   LROVAL  ( lmdns_service_map      ) },
  { LSTRKEY( "__gc"        ),   LFUNCVAL( lmdns_service_gc       ) },
  { LNILKEY, LNILVAL }
};

//called from luaopen_net
LUALIB_API int luaopen_mdns( lua_State *L ) {
  luaL_newmetarotable(L,"mdns.cli", (void *)lmdns_service_map);

#if !LUA_USE_ROTABLE
  luaL_newlib(L, mdns);
  return 1;
#else
	return 0;
#endif
}

#endif

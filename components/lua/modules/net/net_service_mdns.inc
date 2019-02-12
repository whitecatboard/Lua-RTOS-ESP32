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
#include <sys/syslog.h>

#define DEFAULT_TIMEOUT_SECONDS 3
#define DEFAULT_MAXIMUM_RESULTS 20
#define MDNS_NAME_MAX_LEN           64                      // Maximum string length of hostname, instance, service and proto

// Module errors
#define LUA_MDNS_ERR_CANT_START          (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  0)
#define LUA_MDNS_ERR_CANT_STOP           (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  1)
#define LUA_MDNS_ERR_CANT_RESOLVE_HOST   (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  2)
#define LUA_MDNS_ERR_CANT_FIND_SERVICE   (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  3)
#define LUA_MDNS_ERR_CANT_ADD_SERVICE    (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  4)
#define LUA_MDNS_ERR_CANT_REMOVE_SERVICE (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  5)
#define LUA_MDNS_ERR_HOSTNAME_TOO_LONG   (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  6)
#define LUA_MDNS_ERR_INSTANCE_TOO_LONG   (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  7)
#define LUA_MDNS_ERR_SERVICE_TOO_LONG    (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  8)
#define LUA_MDNS_ERR_PROTOCOL_TOO_LONG   (DRIVER_EXCEPTION_BASE(MDNS_DRIVER_ID) |  9)

// Register drivers and errors
DRIVER_REGISTER_BEGIN(MDNS,mdns,0,NULL,NULL);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotCreateService, "can't initialize", LUA_MDNS_ERR_CANT_START);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotStop, "can't stop", LUA_MDNS_ERR_CANT_STOP);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotResolveHost, "can't resolve host", LUA_MDNS_ERR_CANT_RESOLVE_HOST);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotFindService, "can't find service", LUA_MDNS_ERR_CANT_FIND_SERVICE);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotAddService, "can't add service", LUA_MDNS_ERR_CANT_ADD_SERVICE);
	DRIVER_REGISTER_ERROR(MDNS, mdns, CannotRemoveService, "can't remove service", LUA_MDNS_ERR_CANT_REMOVE_SERVICE);
	DRIVER_REGISTER_ERROR(MDNS, mdns, HostnameTooLong, "hostname too long", LUA_MDNS_ERR_HOSTNAME_TOO_LONG);
	DRIVER_REGISTER_ERROR(MDNS, mdns, InstanceTooLong, "instance name too long", LUA_MDNS_ERR_INSTANCE_TOO_LONG);
	DRIVER_REGISTER_ERROR(MDNS, mdns, ServiceTooLong, "service name too long", LUA_MDNS_ERR_SERVICE_TOO_LONG);
	DRIVER_REGISTER_ERROR(MDNS, mdns, ProtocolTooLong, "protocol name too long", LUA_MDNS_ERR_PROTOCOL_TOO_LONG);
DRIVER_REGISTER_END(MDNS,mdns,0,NULL,NULL);

static u8_t _mdns_initialized = 0;
static char *_mdns_hostname = NULL;
static u8_t _mdns_running = 0;

static int lmdns_ensure_init()
{
	if (1 == _mdns_initialized) return 0;
	esp_err_t err = mdns_init();
	if (err == ESP_OK) {
		_mdns_initialized = 1;
	}
	return (err != ESP_OK);
}

static int lmdns_start( lua_State* L ){
	int rc;
	const char *hostname = luaL_optstring( L, 1, "ESP32" );
	const char *instance = luaL_optstring( L, 2, NULL );

	rc = lmdns_ensure_init();
	if (rc != 0) {
		return luaL_exception(L, LUA_MDNS_ERR_CANT_START);
	}

	if(hostname != 0) {
		if (strlen(hostname) > MDNS_NAME_MAX_LEN) {
			return luaL_exception(L, LUA_MDNS_ERR_HOSTNAME_TOO_LONG);
		}

		if (NULL != _mdns_hostname) free(_mdns_hostname);
		_mdns_hostname = (char*)malloc(strlen(hostname)+1);
		if (NULL == _mdns_hostname) {
			syslog(LOG_ERR, "mdns: could not allocate memory for hostname\n");
		}
		else {
			strcpy(_mdns_hostname, hostname);
		}
		rc = mdns_hostname_set(hostname);
		if (rc != 0) {
			syslog(LOG_ERR, "mdns: could not set hostname\n");
		}
	}

	if(instance != 0) {
		if (strlen(hostname) > MDNS_NAME_MAX_LEN) {
			return luaL_exception(L, LUA_MDNS_ERR_HOSTNAME_TOO_LONG);
		}

		rc = mdns_instance_name_set(instance);
		if (rc != 0) {
			syslog(LOG_ERR, "mdns: could not set instance name\n");
		}
	}

	_mdns_running = 1;
	return 0;
}

static void value_to_table(lua_State* L, char* key, char* value){
	lua_pushstring(L, key);
	lua_pushstring(L, (value)?value:"");
	lua_settable(L, -3);
}

static inline void interface_to_table(lua_State* L, char* key, tcpip_adapter_if_t value){
	switch(value) {
		case TCPIP_ADAPTER_IF_STA:
			value_to_table(L, key, "STA");
			break;
		case TCPIP_ADAPTER_IF_AP:
			value_to_table(L, key, "AP");
			break;
		case TCPIP_ADAPTER_IF_ETH:
			value_to_table(L, key, "ETH");
			break;
		case TCPIP_ADAPTER_IF_SPI_ETH:
			value_to_table(L, key, "SPI_ETH");
			break;
		case TCPIP_ADAPTER_IF_TUN:
			value_to_table(L, key, "TUN");
			break;
		default:
			syslog(LOG_ERR, "mdns: unhandled interface type %i\n", (int)value);
	}
}

static inline void protocol_to_table(lua_State* L, char* key, mdns_ip_protocol_t value){
	switch(value) {
		case MDNS_IP_PROTOCOL_V4:
			value_to_table(L, key, "IPv4");
			break;
		case MDNS_IP_PROTOCOL_V6:
			value_to_table(L, key, "IPv6");
			break;
		default:
			syslog(LOG_ERR, "mdns: unhandled protocol type %i\n", (int)value);
	}
}

static int results_to_table(lua_State* L, mdns_result_t * results){
	char tmp[46];
	lua_newtable(L);

	mdns_result_t * r = results;
	mdns_ip_addr_t * a = NULL;
	int row = 1, t;
	while(r){

		//put row into table
		lua_newtable(L);
		lua_pushnumber(L, row++);
		lua_pushvalue(L, -2);
		lua_settable(L, -4);

		interface_to_table(L, "interface", r->tcpip_if);
		protocol_to_table(L, "protocol", r->ip_protocol);

		if(r->instance_name){
			value_to_table(L, "instance", r->instance_name);
		}
		if(r->hostname){
			value_to_table(L, "hostname", r->hostname);
		}
		if(r->port){
			snprintf(tmp, sizeof(tmp), "%i", r->port);
			value_to_table(L, "port", tmp);
		}
		if(r->txt_count){
			lua_pushstring(L, "txt");
			lua_newtable(L);

			for(t=0; t<r->txt_count; t++){
				value_to_table(L, r->txt[t].key, r->txt[t].value);
			}

			lua_settable(L, -3);
		}
		a = r->addr;
		while(a){
			lua_pushstring(L, "ip");
			lua_newtable(L);

			if(a->addr.type == MDNS_IP_PROTOCOL_V6){
				snprintf(tmp, sizeof(tmp), IPV6STR, IPV62STR(a->addr.u_addr.ip6));
				value_to_table(L, "ipv6", tmp);
			} else {
				snprintf(tmp, sizeof(tmp), IPSTR, IP2STR(&(a->addr.u_addr.ip4)));
				value_to_table(L, "ipv4", tmp);
			}

			lua_settable(L, -3);
			a = a->next;
		}
		r = r->next;

		//finish the row
		lua_pop(L, 1);
	}

	return 1; //one table
}

static int lmdns_resolve_host( lua_State* L ) {
	char hostip[46];
	const char *hostname = luaL_checkstring( L, 1 );
	int seconds = luaL_optinteger(L, 2, DEFAULT_TIMEOUT_SECONDS);
	if(seconds == 0) seconds = DEFAULT_TIMEOUT_SECONDS;

	if (strlen(hostname) > MDNS_NAME_MAX_LEN) {
		return luaL_exception(L, LUA_MDNS_ERR_HOSTNAME_TOO_LONG);
	}

	int ipv6 = 0;
	if (lua_gettop(L) > 2) {
		luaL_checktype(L, 3, LUA_TBOOLEAN);
		ipv6 = lua_toboolean( L, 3 );
	}

	int rc = lmdns_ensure_init();
	if (rc != 0) {
		return luaL_exception(L, LUA_MDNS_ERR_CANT_START);
	}

	if (ipv6) {
		ip6_addr_t addr;
		esp_err_t err = mdns_query_aaaa(hostname, 1000*seconds, &addr);
		if(err){
			if(err == ESP_ERR_NOT_FOUND){
				return 0;
			}
			return luaL_exception(L, LUA_MDNS_ERR_CANT_RESOLVE_HOST);
		}
		snprintf(hostip, sizeof(hostip), IPV6STR, IPV62STR(addr));
	}
	else {
		struct ip4_addr addr;
		addr.addr = 0;
		esp_err_t err = mdns_query_a(hostname, 1000*seconds, &addr);
		if(err){
			if(err == ESP_ERR_NOT_FOUND){
				return 0;
			}
			return luaL_exception(L, LUA_MDNS_ERR_CANT_RESOLVE_HOST);
		}

		snprintf(hostip, sizeof(hostip), IPSTR, ip4_addr1_16(&addr),ip4_addr2_16(&addr),ip4_addr3_16(&addr),ip4_addr4_16(&addr));
	}
	lua_pushstring(L, hostip);
	return 1;
}

static int lmdns_find_service( lua_State* L ) {
	int rc = 0;
	mdns_result_t * results = NULL;
	const char *service = luaL_checkstring( L, 1 );
	const char *protocol = luaL_checkstring( L, 2 );
	int seconds = luaL_optinteger(L, 3, DEFAULT_TIMEOUT_SECONDS);
	if(seconds == 0) seconds = DEFAULT_TIMEOUT_SECONDS;
	int maxresult = luaL_optinteger(L, 4, DEFAULT_MAXIMUM_RESULTS);
	if(maxresult == 0) maxresult = DEFAULT_MAXIMUM_RESULTS;

	if (strlen(service) > MDNS_NAME_MAX_LEN) {
		return luaL_exception(L, LUA_MDNS_ERR_SERVICE_TOO_LONG);
	}
	if (strlen(protocol) > MDNS_NAME_MAX_LEN) {
		return luaL_exception(L, LUA_MDNS_ERR_PROTOCOL_TOO_LONG);
	}

	rc = lmdns_ensure_init();
	if (rc != 0) {
		return luaL_exception(L, LUA_MDNS_ERR_CANT_START);
	}

	esp_err_t err = mdns_query_ptr(service, protocol, 1000*seconds, maxresult, &results);
	if(err){
		return luaL_exception(L, LUA_MDNS_ERR_CANT_FIND_SERVICE);
	}

	if(results){
		rc = results_to_table(L, results);
	}

	mdns_query_results_free(results);
	return rc;
}

static int lmdns_add_service( lua_State* L ) {
	int rc;
	const char *service = luaL_checkstring( L, 1 );
	const char *protocol = luaL_checkstring( L, 2 );
	uint16_t port = luaL_checkinteger( L, 3 );
	const char *instance = 0;
	size_t txt_items = 0;
	mdns_txt_item_t * txt_data = 0;

	if (strlen(service) > MDNS_NAME_MAX_LEN) {
		return luaL_exception(L, LUA_MDNS_ERR_SERVICE_TOO_LONG);
	}
	if (strlen(protocol) > MDNS_NAME_MAX_LEN) {
		return luaL_exception(L, LUA_MDNS_ERR_PROTOCOL_TOO_LONG);
	}

	if (lua_gettop(L) > 3) {
		instance = luaL_checkstring( L, 4 );
		if (strlen(instance) > MDNS_NAME_MAX_LEN) {
			return luaL_exception(L, LUA_MDNS_ERR_INSTANCE_TOO_LONG);
		}

		if (lua_gettop(L) > 4 && lua_istable(L, 5)) {

			txt_items = lua_rawlen(L, 5); // size of the txt table
			txt_data = (mdns_txt_item_t *)malloc(sizeof(mdns_txt_item_t) * txt_items);
			txt_items = 0; //restart counting as we may need to discard some non-string items below

			/* table is in the stack at index '5' */
			lua_pushnil(L); /* first key */
			while (lua_next(L, 5) != 0) {
				/* uses 'key' (at index -2) and 'value' (at index -1) */
				if (LUA_TSTRING == lua_type(L, -2) && LUA_TSTRING == lua_type(L, -1)) {
					txt_data[txt_items].key = (char*)lua_tostring(L, -2);
					txt_data[txt_items].value = (char*)lua_tostring(L, -1);
					txt_items++;
				}
				else {
					syslog(LOG_WARNING, "mdns: ignoring txt entry with non-string key and/or value\n");
				}
				/* removes 'value'; keeps 'key' for next iteration */
				lua_pop(L, 1);
			}
		}
	}

	if (0 == _mdns_running) {
		if (0 != txt_data) free(txt_data);
		return luaL_exception(L, LUA_MDNS_ERR_CANT_ADD_SERVICE);
	}

	rc = lmdns_ensure_init();
	if (rc != 0) {
		if (0 != txt_data) free(txt_data);
		return luaL_exception(L, LUA_MDNS_ERR_CANT_START);
	}

	rc = mdns_service_add(instance, service, protocol, port, txt_data, txt_items);
	if (0 != txt_data) free(txt_data);
	if (rc != 0) {
		syslog(LOG_ERR, "mdns: mdns_service_add returned %s\n", esp_err_to_name(rc));
		return luaL_exception(L, LUA_MDNS_ERR_CANT_ADD_SERVICE);
	}

	return 0;
}

static int lmdns_remove_service( lua_State* L ) {
	int rc;
	const char *service = luaL_checkstring( L, 1 );
	const char *protocol = luaL_checkstring( L, 2 );

	if (strlen(service) > MDNS_NAME_MAX_LEN) {
		return luaL_exception(L, LUA_MDNS_ERR_SERVICE_TOO_LONG);
	}
	if (strlen(protocol) > MDNS_NAME_MAX_LEN) {
		return luaL_exception(L, LUA_MDNS_ERR_PROTOCOL_TOO_LONG);
	}

	if (0 == _mdns_running) {
		return luaL_exception(L, LUA_MDNS_ERR_CANT_REMOVE_SERVICE);
	}

	rc = lmdns_ensure_init();
	if (rc != 0) {
		return luaL_exception(L, LUA_MDNS_ERR_CANT_START);
	}

	rc = mdns_service_remove(service, protocol);
	if (rc != 0) {
		return luaL_exception(L, LUA_MDNS_ERR_CANT_REMOVE_SERVICE);
	}
	return 0;
}

static int lmdns_stop( lua_State* L ) {
	int rc = mdns_service_remove_all();

	mdns_free();
	if (NULL != _mdns_hostname) {
		free(_mdns_hostname);
		_mdns_hostname = NULL;
	}
	_mdns_running = 0;
	_mdns_initialized = 0;

	if (rc != 0) {
		return luaL_exception(L, LUA_MDNS_ERR_CANT_STOP);
	}

	return 0;
}

static int lmdns_running( lua_State* L ) {
	lua_pushboolean(L, _mdns_running);
	return 1;
}

static const LUA_REG_TYPE mdns_map[] = {
	{ LSTRKEY( "start"         ), LFUNCVAL( lmdns_start            ) },
	{ LSTRKEY( "stop"          ), LFUNCVAL( lmdns_stop             ) },
	{ LSTRKEY( "running"       ), LFUNCVAL( lmdns_running          ) },
	{ LSTRKEY( "resolvehost"   ), LFUNCVAL( lmdns_resolve_host     ) },
	{ LSTRKEY( "findservice"   ), LFUNCVAL( lmdns_find_service     ) },
	{ LSTRKEY( "addservice"    ), LFUNCVAL( lmdns_add_service      ) },
	{ LSTRKEY( "removeservice" ), LFUNCVAL( lmdns_remove_service   ) },

	// Error definitions
	DRIVER_REGISTER_LUA_ERRORS(mdns)
	{ LNILKEY, LNILVAL }
};

//called from luaopen_net
LUALIB_API int luaopen_mdns( lua_State *L ) {
#if !LUA_USE_ROTABLE
	luaL_newlib(L, mdns);
	return 1;
#else
	return 0;
#endif
}

#endif

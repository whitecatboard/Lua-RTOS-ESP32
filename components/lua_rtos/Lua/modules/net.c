/*
 * Lua RTOS, Lua net module
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

#include "lua.h"
#include "lauxlib.h"
#include "net.h"
#include "error.h"

#include "modules.h"

#include "net_wifi.inc"
#include "net_spi_eth.inc"
#include "net_service_sntp.inc"
#include "net_service_http.inc"
#include "net_service_curl.inc"
#include "net_service_captivedns.inc"
#include "net_ssh.inc"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <lwip/ping.h>
#include "lwip/ip_addr.h"
#include "lwip/ip6_addr.h"

#include <sys/driver.h>
#include <drivers/net.h>

extern LUA_REG_TYPE net_error_map[];

typedef union {
	uint32_t ipaddr;
	uint8_t ipbytes[4];
	uint16_t ipwords[2];
} net_ip;

static int lnet_lookup(lua_State* L) {
	driver_error_t *error;
	struct sockaddr_in address;
	int raw = 0;

	const char* name = luaL_checkstring(L, 1);

	// Check if user wants result as a raw value, or as an string
	if (lua_gettop(L) == 2) {
		luaL_checktype(L, 2, LUA_TBOOLEAN);
		if (lua_toboolean(L, 2)) {
			raw = 1;
		}
	}

	// Resolve name
	if ((error = net_lookup(name, &address))) {
		return luaL_driver_error(L, error);
	}

	if (raw) {
		lua_pushinteger(L, address.sin_addr.s_addr);
	} else {
		lua_pushfstring(L, inet_ntoa(address.sin_addr.s_addr));
	}

	return 1;
}

static int lnet_packip(lua_State *L) {
	net_ip ip;
	unsigned i, temp;

	if (lua_isnumber(L, 1))
		for (i = 0; i < 4; i++) {
			temp = luaL_checkinteger(L, i + 1);
			if ((int)temp < 0 || temp > 255)
		       	return luaL_exception(L, NET_ERR_INVALID_IP);

			ip.ipbytes[i] = temp;
		}
	else {
		const char* pip = luaL_checkstring(L, 1);
		unsigned len, temp[4];

		if (sscanf(pip, "%u.%u.%u.%u%n", temp, temp + 1, temp + 2, temp + 3,
				&len) != 4 || len != strlen(pip))
	       	return luaL_exception(L, NET_ERR_INVALID_IP);

		for (i = 0; i < 4; i++) {
			if (temp[i] > 255)
		       	return luaL_exception(L, NET_ERR_INVALID_IP);

			ip.ipbytes[i] = (uint8_t) temp[i];
		}
	}
	lua_pushinteger(L, ip.ipaddr);
	return 1;
}

static int lnet_unpackip(lua_State *L) {
	net_ip ip;
	unsigned i;
	const char* fmt;

	ip.ipaddr = (uint32_t) luaL_checkinteger(L, 1);
	fmt = luaL_checkstring(L, 2);
	if (!strcmp(fmt, "*n")) {
		for (i = 0; i < 4; i++)
			lua_pushinteger(L, ip.ipbytes[i]);
		return 4;
	} else if (!strcmp(fmt, "*s")) {
		lua_pushfstring(L, "%d.%d.%d.%d", (int) ip.ipbytes[0],
				(int) ip.ipbytes[1], (int) ip.ipbytes[2], (int) ip.ipbytes[3]);
		return 1;
	} else
		return luaL_error(L, "invalid format");
}

static int lnet_ping(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	int top = lua_gettop(L);
	int i, count = 0, interval = 0, size = 0, timeout = 0;

	for (i = 1; i <= top; i++) {
		switch (i) {
		case 2:
			count = luaL_optinteger(L, i, 0);
			break;
		case 3:
			interval = luaL_optinteger(L, i, 0);
			break;
		case 4:
			size = luaL_optinteger(L, i, 0);
			break;
		case 5:
			timeout = luaL_optinteger(L, i, 0);
			break;
		}
	}

	ping(name, count, interval, size, timeout);

	return 0;
}

static int lnet_stat(lua_State* L) {
	u8_t table = 0;

	// Check if user wants result as a table, or wants scan's result
	// on the console
	if (lua_gettop(L) == 1) {
		luaL_checktype(L, 1, LUA_TBOOLEAN);
		if (lua_toboolean(L, 1)) {
			table = 1;
		}
	}

	if (table) {
		lua_createtable(L, 1, 0);
		lua_pushinteger(L, 0);
	}

	// This should be done in a more elegant way in future versions ...

#if CONFIG_WIFI_ENABLED && CONFIG_LUA_RTOS_LUA_USE_NET
	// Call wf.stat
	lua_getglobal(L, "net");
	lua_getfield(L, -1, "wf");
	lua_getfield(L, -1, "stat");
	lua_pushboolean(L, table);

	if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
		return luaL_error(L, lua_tostring(L, -1));
	}

	if (table) {
		lua_remove(L, 4);
		lua_remove(L, 4);
		lua_settable(L, -3);
	} else {
		lua_settop(L, 0);
	}

	if (table) {
		lua_pushinteger(L, 1);
	}
#endif

#if CONFIG_SPI_ETHERNET && CONFIG_LUA_RTOS_LUA_USE_NET
	// Call wf.stat
	lua_getglobal(L, "net");
	lua_getfield(L, -1, "en");
	lua_getfield(L, -1, "stat");
	lua_pushboolean(L, table);

	if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
		return luaL_error(L, lua_tostring(L, -1));
	}

	if (table) {
		lua_remove(L, 4);
		lua_remove(L, 4);
		lua_settable(L, -3);
	} else {
		lua_settop(L, 0);
	}
#endif

	return table;
}

static const LUA_REG_TYPE service_map[] = {
	{ LSTRKEY( "sntp" ), LROVAL ( sntp_map ) },
#if LUA_USE_HTTP
	{ LSTRKEY( "http" ), LROVAL ( http_map ) },
	{ LSTRKEY( "captivedns" ), LROVAL ( captivedns_map ) },
#endif
};

static const LUA_REG_TYPE net_map[] = {
	{ LSTRKEY( "stat" ), LFUNCVAL ( lnet_stat ) },
	{ LSTRKEY( "lookup" ), LFUNCVAL ( lnet_lookup ) },
	{ LSTRKEY( "packip" ), LFUNCVAL ( lnet_packip ) },
	{ LSTRKEY( "unpackip" ), LFUNCVAL ( lnet_unpackip ) },
	{ LSTRKEY( "ping" ), LFUNCVAL ( lnet_ping ) },

#if CONFIG_LUA_RTOS_LUA_USE_SCP_NET
	{ LSTRKEY( "scp" ), LROVAL ( scp_map ) },
#endif

#if CONFIG_WIFI_ENABLED && CONFIG_LUA_RTOS_LUA_USE_NET
	{ LSTRKEY( "wf" ), LROVAL ( wifi_map ) },
#endif

	#if CONFIG_SPI_ETHERNET && CONFIG_LUA_RTOS_LUA_USE_NET
	{ LSTRKEY( "en" ), LROVAL ( spi_eth_map ) },
#endif

#if CONFIG_LUA_RTOS_LUA_USE_CURL_NET
	{ LSTRKEY( "curl" ), LROVAL ( curl_map ) },
#endif

	{ LSTRKEY( "service" ), LROVAL ( service_map ) },
	{ LSTRKEY( "error" ), LROVAL ( net_error_map ) },
	{ LNILKEY, LNILVAL }
};

int luaopen_net(lua_State* L) {
	return 0;
}

MODULE_REGISTER_MAPPED(NET, net, net_map, luaopen_net);

/*

 net.wf.setup(net.wf.mode.STA, "CITILAB","wifi@citilab")
 net.wf.start()
 net.service.sntp.start()
 net.service.sntp.stop()

 net.lookup("whitecarboard.org")
 */

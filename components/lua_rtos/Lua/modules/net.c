/*
 * Lua RTOS, Lua net module
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"
#include "net.h"
#include "error.h"

#include "modules.h"

#include "net_wifi.inc"
#include "net_service_sntp.inc"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

static int lnet_lookup(lua_State* L) {
	const char* name = luaL_checkstring( L, 1 );
	int port = 0;
	struct sockaddr_in address;
	int rc = 0;
	int raw = 0;

	sa_family_t family = AF_INET;
	struct addrinfo *result = NULL;
	struct addrinfo hints = {0, AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

	// Check if user wants result as a raw value, or as an string
	if (lua_gettop(L) == 2) {
		luaL_checktype(L, 2, LUA_TBOOLEAN);
		if (lua_toboolean(L, 2)) {
			raw = 1;
		}
	}

	if ((rc = getaddrinfo(name, NULL, &hints, &result)) == 0) {
		struct addrinfo *res = result;
		while (res) {
			if (res->ai_family == AF_INET) {
				result = res;
				break;
			}
			res = res->ai_next;
		}

		if (result->ai_family == AF_INET) {
			address.sin_port = htons(port);
			address.sin_family = family = AF_INET;
            address.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
		} else {
            rc = -1;
        }

        freeaddrinfo(result);
	}

	if (rc == 0) {
		if (raw) {
			lua_pushinteger( L, address.sin_addr.s_addr);
		} else {
		    lua_pushfstring(L, inet_ntoa(address.sin_addr.s_addr));
		}
	} else {
		lua_pushnil(L);
	}

	return 1;
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
    	lua_settable(L,-3);
    } else {
    	lua_settop(L, 0);
    }

	return table;
}

static const LUA_REG_TYPE service_map[] = {
	{ LSTRKEY( "sntp"    ),	 LROVAL   ( sntp_map  ) },
};

static const LUA_REG_TYPE net_map[] = {
	{ LSTRKEY( "stat"       ),	 LFUNCVAL ( lnet_stat     ) },
	{ LSTRKEY( "lookup"     ),	 LFUNCVAL ( lnet_lookup   ) },
	{ LSTRKEY( "wf"         ),	 LROVAL   ( wifi_map      ) },
	{ LSTRKEY( "service"    ),	 LROVAL   ( service_map   ) },
	{ LNILKEY, LNILVAL }
};

int luaopen_net(lua_State* L) {
    return 0;
}

LUA_OS_MODULE(NET, net, net_map);

/*

net.wf.setup(net.wf.mode.STA, "CITILAB","wifi@citilab")
net.wf.start()
net.service.sntp.start()
net.service.sntp.stop()

net.lookup("whitecarboard.org")
 */

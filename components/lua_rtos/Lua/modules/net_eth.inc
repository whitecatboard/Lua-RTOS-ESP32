/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, Lua ethernet net module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET && CONFIG_LUA_RTOS_ETH_HW_TYPE_RMII

#include <string.h>
#include <stdio.h>

#include <drivers/eth.h>

static int leth_setup(lua_State* L) {
	driver_error_t *error;

	uint32_t ip = luaL_optinteger(L, 1, 0);
	uint32_t mask = luaL_optinteger(L, 2, 0);
	uint32_t gw = luaL_optinteger(L, 3, 0);
	uint32_t dns1 = luaL_optinteger(L, 4, 0);
	uint32_t dns2 = luaL_optinteger(L, 5, 0);

    // Setup eth
	if ((error = eth_setup(ip, mask, gw, dns1, dns2))) {
    	return luaL_driver_error(L, error);
	}

	return 0;
}

static int leth_start(lua_State* L) {
	driver_error_t *error;
	uint8_t silent = 1;

	if (lua_gettop(L) == 1) {
		luaL_checktype(L, 1, LUA_TBOOLEAN);
		if (lua_toboolean(L, 1)) {
			silent = 1;
		}
	}

	if ((error = eth_start(silent))) {
    	return luaL_driver_error(L, error);
	}

	return 0;
}

static int leth_stop(lua_State* L) {
	driver_error_t *error;

	if ((error = eth_stop())) {
    	return luaL_driver_error(L, error);
	}

	return 0;
}

static int leth_stat(lua_State* L) {
	ifconfig_t info;
	driver_error_t *error;
	u8_t table = 0;

	// Check if user wants result as a table, or wants scan's result
	// on the console
	if (lua_gettop(L) == 1) {
		luaL_checktype(L, 1, LUA_TBOOLEAN);
		if (lua_toboolean(L, 1)) {
			table = 1;
		}
	}

	if ((error = eth_stat(&info))) {
		if (error->exception != ETH_ERR_NOT_INIT) {
			return luaL_driver_error(L, error);
		}

		free(error);

		memset(&info, 0, sizeof(ifconfig_t));
	}

	if (!table) {
		    char ipa[16];
		    char maska[16];
		    char gwa[16];

		    strcpy(ipa, inet_ntoa(info.ip));
		    strcpy(maska, inet_ntoa(info.netmask));
		    strcpy(gwa, inet_ntoa(info.gw));

			printf(
					"en: mac address %02x:%02x:%02x:%02x:%02x:%02x\r\n",
					info.mac[0],info.mac[1],
					info.mac[2],info.mac[3],
					info.mac[4],info.mac[5]
			);

			printf("   ip address %s netmask %s\r\n", ipa, maska);
			printf("   gw address %s\r\n\r\n", gwa);
	} else {
		char tmp[18];

		lua_createtable(L, 0, 4);

        lua_pushstring(L, "en");
        lua_setfield (L, -2, "interface");

        sprintf(tmp,"%d.%d.%d.%d", ip4_addr1_16(&info.ip),ip4_addr2_16(&info.ip),ip4_addr3_16(&info.ip),ip4_addr4_16(&info.ip));
        lua_pushstring(L, tmp);
        lua_setfield (L, -2, "ip");

		sprintf(tmp,"%d.%d.%d.%d", ip4_addr1_16(&info.gw),ip4_addr2_16(&info.gw),ip4_addr3_16(&info.gw),ip4_addr4_16(&info.gw));
        lua_pushstring(L, tmp);
        lua_setfield (L, -2, "gw");

		sprintf(tmp,"%d.%d.%d.%d", ip4_addr1_16(&info.netmask),ip4_addr2_16(&info.netmask),ip4_addr3_16(&info.netmask),ip4_addr4_16(&info.netmask));
        lua_pushstring(L, tmp);
        lua_setfield (L, -2, "netmask");

		sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x",
			info.mac[0],info.mac[1],
			info.mac[2],info.mac[3],
			info.mac[4],info.mac[5]
		);
		lua_pushstring(L, tmp);
		lua_setfield (L, -2, "mac");
	}

	return table;
}

static const LUA_REG_TYPE eth_map[] = {
	{ LSTRKEY( "setup" ),	 LFUNCVAL( leth_setup   ) },
    { LSTRKEY( "start" ),	 LFUNCVAL( leth_start   ) },
    { LSTRKEY( "stop"  ),	 LFUNCVAL( leth_stop    ) },
    { LSTRKEY( "stat"  ),	 LFUNCVAL( leth_stat    ) },
	DRIVER_REGISTER_LUA_ERRORS(eth)
	{ LNILKEY, LNILVAL }
};

#endif

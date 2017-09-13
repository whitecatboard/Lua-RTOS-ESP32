/*
 * Lua RTOS, SPI ethernet module
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET && CONFIG_SPI_ETHERNET

#include <string.h>
#include <stdio.h>

#include <drivers/spi_eth.h>

static int lspi_eth_setup(lua_State* L) {
	driver_error_t *error;

	uint32_t ip = luaL_optinteger(L, 1, 0);
	uint32_t mask = luaL_optinteger(L, 2, 0);
	uint32_t gw = luaL_optinteger(L, 3, 0);
	uint32_t dns1 = luaL_optinteger(L, 4, 0);
	uint32_t dns2 = luaL_optinteger(L, 5, 0);

    // Setup spi_eth
	if ((error = spi_eth_setup(ip, mask, gw, dns1, dns2))) {
    	return luaL_driver_error(L, error);
	}

	return 0;
}

static int lspi_eth_start(lua_State* L) {
	driver_error_t *error;

	if ((error = spi_eth_start())) {
    	return luaL_driver_error(L, error);
	}

	return 0;
}

static int lspi_eth_stop(lua_State* L) {
	driver_error_t *error;

	if ((error = spi_eth_stop())) {
    	return luaL_driver_error(L, error);
	}

	return 0;
}

static int lspi_eth_stat(lua_State* L) {
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

	if ((error = spi_eth_stat(&info))) {
		if (error->exception != SPI_ETH_ERR_NOT_INIT) {
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

static const LUA_REG_TYPE spi_eth_map[] = {
	{ LSTRKEY( "setup" ),	 LFUNCVAL( lspi_eth_setup   ) },
    { LSTRKEY( "start" ),	 LFUNCVAL( lspi_eth_start   ) },
    { LSTRKEY( "stop"  ),	 LFUNCVAL( lspi_eth_stop    ) },
    { LSTRKEY( "stat"  ),	 LFUNCVAL( lspi_eth_stat    ) },
	DRIVER_REGISTER_LUA_ERRORS(spi_eth)
	{ LNILKEY, LNILVAL }
};

#endif

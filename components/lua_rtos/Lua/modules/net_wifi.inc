/*
 * Lua RTOS, wifi module
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

#if CONFIG_WIFI_ENABLED && CONFIG_LUA_RTOS_LUA_USE_NET

#include <string.h>
#include <stdio.h>

extern LUA_REG_TYPE wifi_error_map[];

static int lwifi_setup(lua_State* L) {
	driver_error_t *error;

	int mode = luaL_checkinteger(L, 1);
	const char *ssid = luaL_checkstring(L, 2);
	const char *password = luaL_checkstring(L, 3);
	int powersave = 0;
	int channel = 0;
	int hidden = 0;


	if (lua_gettop(L) > 3) {
		powersave = luaL_checkinteger(L, 4);

		if (lua_gettop(L) > 4) {
			channel = luaL_checkinteger(L, 5);

			// Get arguments related only to AP mode
			if (mode == WIFI_MODE_STA) {
				if (lua_gettop(L) > 5) {
					luaL_checktype(L, 6, LUA_TBOOLEAN);
					if (lua_toboolean(L, 6)) {
						hidden = 1;
					}
				}
			}
		}
	}

	// Setup wifi
	if ((error = wifi_setup(mode, (char *)ssid, (char *)password, powersave, channel, hidden))) {
		return luaL_driver_error(L, error);
	}

	return 0;
}

static int lwifi_scan(lua_State* L) {
	driver_error_t *error;
	wifi_ap_record_t *list;
	uint16_t count, i;
	u8_t table = 0;
	char auth[13];

	// Check if user wants scan's result as a table, or wants scan's result
	// on the console
	if (lua_gettop(L) == 1) {
		luaL_checktype(L, 1, LUA_TBOOLEAN);
		if (lua_toboolean(L, 1)) {
			table = 1;
		}
	}

	// Scan wifi
	if ((error = wifi_scan(&count, &list))) {
		return luaL_driver_error(L, error);
	}

	// Show / get APs
	if (!table) {
		printf("\r\n                           SSID  RSSI          AUTH\r\n");
		printf("---------------------------------------------------\r\n");
	} else {
		lua_createtable(L, count, 0);
	}

	for(i=0;i<count;i++) {
		if (!table) {
			switch (list[i].authmode) {
				case WIFI_AUTH_OPEN: strcpy(auth, "OPEN"); break;
				case WIFI_AUTH_WEP: strcpy(auth, "WEP"); break;
				case WIFI_AUTH_WPA_PSK: strcpy(auth, "WPA_PSK"); break;
				case WIFI_AUTH_WPA2_PSK: strcpy(auth, "WPA2_PSK"); break;
				case WIFI_AUTH_WPA_WPA2_PSK: strcpy(auth, "WPA_WPA2_PSK"); break;
				default:
					break;
			}

			printf("%31.31s  %4d %13.13s\r\n",list[i].ssid, list[i].rssi, auth);
		} else {
			lua_pushinteger(L, i);

			lua_createtable(L, 0, 3);

			lua_pushstring(L, (char *)list[i].ssid);
			lua_setfield (L, -2, "ssid");

			lua_pushinteger(L, list[i].rssi);
			lua_setfield (L, -2, "rssi");

			lua_pushinteger(L, list[i].authmode);
			lua_setfield (L, -2, "auth");

			lua_settable(L,-3);
		}
	}

	free(list);

	if (!table) {
		printf("\r\n");
	}

	return table;
}

static int lwifi_start(lua_State* L) {
	driver_error_t *error;

	if ((error = wifi_start())) {
		return luaL_driver_error(L, error);
	}

	return 0;
}

static int lwifi_stop(lua_State* L) {
	driver_error_t *error;

	if ((error = wifi_stop())) {
		return luaL_driver_error(L, error);
	}

	return 0;
}

static int lwifi_stat(lua_State* L) {
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

	if ((error = wifi_stat(&info))) {
		if (error->exception != WIFI_ERR_WIFI_NOT_INIT) {
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

		printf( "wf: mac address %02x:%02x:%02x:%02x:%02x:%02x\r\n",
			info.mac[0],info.mac[1],
			info.mac[2],info.mac[3],
			info.mac[4],info.mac[5]
		);

		printf("   ip address %s netmask %s\r\n", ipa, maska);
		printf("   gw address %s\r\n\r\n", gwa);
	} else {
		char tmp[17];

		lua_createtable(L, 0, 4);

		lua_pushstring(L, "wf");
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
	}

	return table;
}

static const LUA_REG_TYPE wifi_auth_map[] = {
	{ LSTRKEY( "OPEN"         ), LINTVAL( WIFI_AUTH_OPEN ) },
	{ LSTRKEY( "WEP"          ), LINTVAL( WIFI_AUTH_WEP ) },
	{ LSTRKEY( "WPA_PSK"      ), LINTVAL( WIFI_AUTH_WPA_PSK ) },
	{ LSTRKEY( "WPA2_PSK"     ), LINTVAL( WIFI_AUTH_WPA2_PSK ) },
	{ LSTRKEY( "WPA_WPA2_PSK" ), LINTVAL( WIFI_AUTH_WPA_WPA2_PSK ) },
};

static const LUA_REG_TYPE wifi_mode_map[] = {
	{ LSTRKEY( "STA"          ), LINTVAL( WIFI_MODE_STA ) },
	{ LSTRKEY( "AP"           ), LINTVAL( WIFI_MODE_AP ) },
	{ LSTRKEY( "APSTA"        ), LINTVAL( WIFI_MODE_APSTA ) },
};

static const LUA_REG_TYPE wifi_powersave_map[] = {
	{ LSTRKEY( "NONE"          ), LINTVAL( WIFI_PS_NONE ) },
	{ LSTRKEY( "MODEM"         ), LINTVAL( WIFI_PS_MODEM ) },
};

static const LUA_REG_TYPE wifi_map[] = {
	{ LSTRKEY( "setup"     ),	 LFUNCVAL( lwifi_setup        ) },
	{ LSTRKEY( "scan"      ),	 LFUNCVAL( lwifi_scan         ) },
	{ LSTRKEY( "start"     ),	 LFUNCVAL( lwifi_start        ) },
	{ LSTRKEY( "stop"      ),	 LFUNCVAL( lwifi_stop         ) },
	{ LSTRKEY( "stat"      ),	 LFUNCVAL( lwifi_stat         ) },
	{ LSTRKEY( "auth"      ),	 LROVAL  ( wifi_auth_map      ) },
	{ LSTRKEY( "mode"      ),	 LROVAL  ( wifi_mode_map      ) },
	{ LSTRKEY( "powersave" ),	 LROVAL  ( wifi_powersave_map ) },
	{ LSTRKEY( "error"     ),	 LROVAL  ( wifi_error_map     ) },
	{ LNILKEY, LNILVAL }
};

#endif

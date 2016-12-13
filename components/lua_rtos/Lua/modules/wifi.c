/*
 * Lua RTOS, wifi module
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

#if LUA_USE_WIFI

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"
#include "wifi.h"
#include "error.h"

#include "modules.h"

#include <drivers/wifi.h>

static int lwifi_setup(lua_State* L) {
	driver_error_t *error;

	int mode = luaL_checkinteger(L, 1);
    char *ssid = NULL;
    char *password = NULL;

    // Get arguments related to desired mode
    if (mode == WIFI_MODE_STA) {
    	ssid = luaL_checkstring(L, 2);
    	password = luaL_checkstring(L, 3);
    }

    // Setup wifi
	if ((error = wifi_setup(mode, ssid, password))) {
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


#if LUA_USE_ROTABLE
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
#endif

static const LUA_REG_TYPE wifi_map[] = {
	{ LSTRKEY( "setup" ),	 LFUNCVAL( lwifi_setup   ) },
    { LSTRKEY( "scan"  ),	 LFUNCVAL( lwifi_scan    ) },
    { LSTRKEY( "start" ),	 LFUNCVAL( lwifi_start   ) },
    { LSTRKEY( "stop"  ),	 LFUNCVAL( lwifi_stop    ) },
#if LUA_USE_ROTABLE
    { LSTRKEY( "auth"  ),	 LROVAL  ( wifi_auth_map ) },
    { LSTRKEY( "mode"  ),	 LROVAL  ( wifi_mode_map ) },
#endif
	{ LNILKEY, LNILVAL }
};

int luaopen_wifi(lua_State* L) {
#if !LUA_USE_ROTABLE
    luaL_newlib(L, wifi_map);

    return 1;
#else
    return 0;
#endif
}

LUA_OS_MODULE(WIFI, wifi, wifi_map);

#endif

/*

wifi.setup(wifi.mode.STA, "Xarxa Wi-Fi de JAUME","castellbisbal123")
wifi.start()

 */

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
 * Lua RTOS, Lua wifi net module
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET

#include <string.h>
#include <stdio.h>
#include <esp_wps.h>

static int wps_callback = LUA_NOREF;
static int sc_callback = LUA_NOREF;
static lua_State* LL;

static int lwifi_setup(lua_State* L) {
	driver_error_t *error;

	int powersave = 0;
	int channel = 0;
	int hidden = 0;

	uint32_t ip = 0;
	uint32_t mask = 0;
	uint32_t gw = 0;
	uint32_t dns1 = 0;
	uint32_t dns2 = 0;

	int mode = luaL_checkinteger(L, 1);
	const char *ssid = luaL_checkstring(L, 2);
	const char *password = luaL_checkstring(L, 3);

	if (mode == WIFI_MODE_STA) {
		// Get IP info, if present
		ip = luaL_optinteger(L, 4, 0);
		mask = luaL_optinteger(L, 5, 0);
		gw = luaL_optinteger(L, 6, 0);
		dns1 = luaL_optinteger(L, 7, 0);
		dns2 = luaL_optinteger(L, 8, 0);
		powersave = luaL_optinteger(L, 9, 0);
		channel = luaL_optinteger(L, 10, 0);
	} else if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
		powersave = luaL_optinteger(L, 4, 0);
		channel = luaL_optinteger(L, 5, 0);

		if (lua_gettop(L) > 5) {
			luaL_checktype(L, 6, LUA_TBOOLEAN);
			if (lua_toboolean(L, 6)) {
				hidden = 1;
			}
		}
	}

	// Setup wifi
	if ((error = wifi_setup(mode, (char *)ssid, (char *)password, ip, mask, gw, dns1, dns2, powersave, channel, hidden))) {
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

		printf("   ip address %s / netmask %s\r\n", ipa, maska);
		printf("   gw address %s\r\n", gwa);

		if (!ip6_addr_cmp(IP6_ADDR_ANY6, &info.ip6)) {
			char tmp[46];
			snprintf(tmp, sizeof(tmp), IPV6STR, IPV62STR(info.ip6));
			printf("  ip6 address %s\r\n", tmp);
		}

		printf("\r\n");

	} else {
		char tmp[46];
		tmp[0] = 0;

		if (!ip6_addr_cmp(IP6_ADDR_ANY6, &info.ip6)) {
			snprintf(tmp, sizeof(tmp), IPV6STR, IPV62STR(info.ip6));
		}

		lua_createtable(L, 0, strlen(tmp) > 0 ? 6:5);

		lua_pushstring(L, "wf");
		lua_setfield (L, -2, "interface");

		snprintf(tmp, sizeof(tmp), IPSTR, ip4_addr1_16(&info.ip),ip4_addr2_16(&info.ip),ip4_addr3_16(&info.ip),ip4_addr4_16(&info.ip));
		lua_pushstring(L, tmp);
		lua_setfield (L, -2, "ip");

		snprintf(tmp, sizeof(tmp), IPSTR, ip4_addr1_16(&info.gw),ip4_addr2_16(&info.gw),ip4_addr3_16(&info.gw),ip4_addr4_16(&info.gw));
		lua_pushstring(L, tmp);
		lua_setfield (L, -2, "gw");

		snprintf(tmp, sizeof(tmp), IPSTR, ip4_addr1_16(&info.netmask),ip4_addr2_16(&info.netmask),ip4_addr3_16(&info.netmask),ip4_addr4_16(&info.netmask));
		lua_pushstring(L, tmp);
		lua_setfield (L, -2, "netmask");

		snprintf(tmp, sizeof(tmp), "%02x:%02x:%02x:%02x:%02x:%02x",
			info.mac[0],info.mac[1],
			info.mac[2],info.mac[3],
			info.mac[4],info.mac[5]
		);
		lua_pushstring(L, tmp);
		lua_setfield (L, -2, "mac");

		if (strlen(tmp) > 0 ) {
			lua_pushstring(L, tmp);
			lua_setfield (L, -2, "ip6");
		}
	}

	return table;
}

static void lwifi_wps_pin(char* pin) {
	if (wps_callback != LUA_NOREF) {
		lua_rawgeti(LL, LUA_REGISTRYINDEX, wps_callback);
		lua_pushstring(LL, pin);
		lua_call(LL, 1, 0);
	}
	free(pin);
}

static int lwifi_wps(lua_State* L) {
	driver_error_t *error;

	LL = L;
	if (wps_callback != LUA_NOREF) {
		luaL_unref(L, LUA_REGISTRYINDEX, wps_callback);
		wps_callback = LUA_NOREF;
	}

	int wpsmode = luaL_optinteger(L, 1, WPS_TYPE_PBC);
	if (wpsmode != WPS_TYPE_PBC) {

		luaL_checktype(L, 2, LUA_TFUNCTION);
		// Copy argument (function) to the top of stack
		lua_pushvalue(L, 2);
		// Copy function reference
		wps_callback = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	if ((error = wifi_wps(wpsmode, lwifi_wps_pin))) {
		return luaL_driver_error(L, error);
	}

	return 0;
}

static void lwifi_sc_config(char* ssid, char* password) {
	if (sc_callback != LUA_NOREF) {
		lua_rawgeti(LL, LUA_REGISTRYINDEX, sc_callback);
		lua_pushstring(LL, ssid);
		lua_pushstring(LL, password);
		lua_call(LL, 2, 0);
	}
	/* ssid and password are allocated (and freed) by esp_smartconfig_... */
}

static int lwifi_smartconfig(lua_State* L) {
	driver_error_t *error;

	LL = L;
	if (sc_callback != LUA_NOREF) {
		luaL_unref(L, LUA_REGISTRYINDEX, sc_callback);
		sc_callback = LUA_NOREF;
	}

	luaL_checktype(L, 1, LUA_TFUNCTION);
	// Copy argument (function) to the top of stack
	lua_pushvalue(L, 1);
	// Copy function reference
	sc_callback = luaL_ref(L, LUA_REGISTRYINDEX);

	if ((error = wifi_smartconfig(lwifi_sc_config))) {
		return luaL_driver_error(L, error);
	}

	return 0;
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

static const LUA_REG_TYPE wifi_wpstype_map[] = {
	{ LSTRKEY( "DISABLE"       ), LINTVAL( WPS_TYPE_DISABLE ) },
	{ LSTRKEY( "PBC"           ), LINTVAL( WPS_TYPE_PBC ) },
	{ LSTRKEY( "PIN"           ), LINTVAL( WPS_TYPE_PIN ) },
};

static const LUA_REG_TYPE wifi_map[] = {
	{ LSTRKEY( "setup"     ),	 LFUNCVAL( lwifi_setup        ) },
	{ LSTRKEY( "scan"      ),	 LFUNCVAL( lwifi_scan         ) },
	{ LSTRKEY( "start"     ),	 LFUNCVAL( lwifi_start        ) },
	{ LSTRKEY( "stop"      ),	 LFUNCVAL( lwifi_stop         ) },
	{ LSTRKEY( "stat"      ),	 LFUNCVAL( lwifi_stat         ) },
#if CONFIG_ESP32_WIFI_NVS_ENABLED
	// wps ssid+auth can only be stored to nvs
	// so if nvs is disabled, wps would have to be repeated on every boot
	{ LSTRKEY( "startwps"  ),	 LFUNCVAL( lwifi_wps          ) },
#endif
	{ LSTRKEY( "startsc"   ),	 LFUNCVAL( lwifi_smartconfig  ) },
	{ LSTRKEY( "auth"      ),	 LROVAL  ( wifi_auth_map      ) },
	{ LSTRKEY( "mode"      ),	 LROVAL  ( wifi_mode_map      ) },
	{ LSTRKEY( "powersave" ),	 LROVAL  ( wifi_powersave_map ) },
	{ LSTRKEY( "wpstype"   ),	 LROVAL  ( wifi_wpstype_map   ) },
	DRIVER_REGISTER_LUA_ERRORS(wifi)
	{ LNILKEY, LNILVAL }
};

#endif

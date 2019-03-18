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
 * Lua RTOS, Lua LORA WAN module
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276 || CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272 || CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1301

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "modules.h"
#include "error.h"
#include "hex.h"

#include <string.h>
#include <stdlib.h>

#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1301
void lora_gw_start();
#endif

#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276 || CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272
#include <lora/node/lmic/lora.h>
#include <lora/gateway/single_channel/gateway.h>

#include <drivers/uart.h>

static int rx_callback = 0;
static lua_State* rx_callbackL;
static uint8_t is_gateway = 0;

static void on_received(int port, char *payload) {
    if (rx_callback != LUA_NOREF) {
        lua_rawgeti(rx_callbackL, LUA_REGISTRYINDEX, rx_callback);
        lua_pushinteger(rx_callbackL, port);
        lua_pushlstring(rx_callbackL, payload, strlen(payload));
        lua_call(rx_callbackL, 2, 0);
    }

    free(payload);
}

// Pads a hex number string representation at a specified length
static char *hex_str_pad(lua_State* L, const char  *str, int len) {
    if (!lcheck_hex_str(str)) {
        luaL_error(L, "invalid hexadecimal number");
    }

    // Allocate string
    char *tmp = (char *)malloc(len + 1);
    if (!tmp) {
        luaL_error(L, "not enough memory");
    }

    if (strlen(str) < len) {
        // Needs pad
        int i;
        int curr_len = strlen(str);
        int pad_num = len - curr_len;
        char *c = tmp;

        // Pad with 0
        for(i=0;i < pad_num;i++) {
            *c++ = '0';
        }

       // Copy rest of string
        for(i = pad_num - 1;i < len; i++) {
            *c++ = *str++;
        }

        *c = 0x00;
    } else {
        strcpy(tmp, str);
    }

    return tmp;
}
#endif

static int llora_attach(lua_State* L) {
#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276
	driver_error_t *error;
    int type;
    const char *host;
    int port, freq, drate;

    int band = luaL_checkinteger(L, 1);
    type = luaL_optinteger(L, 2, 0);

	if (type == 0) {
		// Node
	    error = lora_setup(band);
	    if (error) {
	        return luaL_driver_error(L, error);
	    }

	    is_gateway = 0;

	} else if (type == 1) {
		host = luaL_optstring(L, 3, "router.eu.thethings.network");
		port = luaL_optinteger(L, 4, 1700);
        freq = luaL_optinteger(L, 5, 868100000);
        drate = luaL_optinteger(L, 6, 5);

		// Gateway
		if ((error = lora_gw_setup(band, host, port, freq, drate))) {
			return luaL_driver_error(L, error);
		}

		is_gateway = 1;
	} else {
		luaL_exception_extended(L, LORA_ERR_CANT_SETUP, "invalid type");
	}
#endif
#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1301
	lora_gw_start();
#endif
    return 0;
}

#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276 || CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272
static int llora_set_setDevAddr(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

    char *devAddr = hex_str_pad(L, luaL_checkstring(L, 1), 8);

    driver_error_t *error = lora_mac_set(LORA_MAC_SET_DEVADDR, devAddr);
    if (error) {
        free(devAddr);
        return luaL_driver_error(L, error);
    }

    free(devAddr);
    return 0;
}

static int llora_set_DevEui(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char  *devEui = hex_str_pad(L, luaL_checkstring(L, 1), 16);

    driver_error_t *error = lora_mac_set(LORA_MAC_SET_DEVEUI, devEui);
    if (error) {
        free(devEui);
        return luaL_driver_error(L, error);
    }

    free(devEui);
    return 0;
}

static int llora_set_AppEui(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char  *appEui = hex_str_pad(L, luaL_checkstring(L, 1), 16);

    driver_error_t *error = lora_mac_set(LORA_MAC_SET_APPEUI, appEui);
    if (error) {
        free(appEui);
        return luaL_driver_error(L, error);
    }

    free(appEui);
    return 0;
}

static int llora_set_NwkSKey(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char  *nwkSKey = hex_str_pad(L, luaL_checkstring(L, 1), 32);

    driver_error_t *error = lora_mac_set(LORA_MAC_SET_NWKSKEY, nwkSKey);
    if (error) {
        free(nwkSKey);
        return luaL_driver_error(L, error);
    }

    free(nwkSKey);
    return 0;
}

static int llora_set_AppSKey(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char  *appSKey = hex_str_pad(L, luaL_checkstring(L, 1), 32);

    driver_error_t *error = lora_mac_set(LORA_MAC_SET_APPSKEY, appSKey);
    if (error) {
        free(appSKey);
        return luaL_driver_error(L, error);
    }

    free(appSKey);
    return 0;
}

static int llora_set_AppKey(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char  *appKey = hex_str_pad(L, luaL_checkstring(L, 1), 32);

    driver_error_t *error = lora_mac_set(LORA_MAC_SET_APPKEY, appKey);
    if (error) {
        free(appKey);
        return luaL_driver_error(L, error);
    }

    free(appKey);
    return 0;
}

static int llora_set_Dr(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	int dr = luaL_checkinteger(L, 1);

    if ((dr < 0) || (dr > 7)) {
        return luaL_error(L, "%d:invalid data rate value (0 to 7)", LORA_ERR_INVALID_ARGUMENT);
    }

    char value[2];

    sprintf(value,"%d", dr);

    driver_error_t *error = lora_mac_set(LORA_MAC_SET_DR, value);
    if (error) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

static int llora_set_Adr(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char value[4];

    luaL_checktype(L, 1, LUA_TBOOLEAN);
    if (lua_toboolean( L, 1 )) {
        strcpy(value, "on");
    } else {
        strcpy(value, "off");
    }

    driver_error_t *error = lora_mac_set(LORA_MAC_SET_ADR, value);
    if (error) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int llora_set_ReTx(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	int rets = luaL_checkinteger(L, 1);

    if ((rets < 0) || (rets > 7)) {
        return luaL_error(L, "%d:invalid data rate value (0 to 8)", LORA_ERR_INVALID_ARGUMENT);
    }

    char value[2];

    sprintf(value,"%d", rets);

    driver_error_t *error = lora_mac_set(LORA_MAC_SET_RETX, value);
    if (error) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

static int llora_get_DevAddr(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char *value;

    driver_error_t *error = lora_mac_get(LORA_MAC_GET_DEVADDR, &value);
    if (error) {
    	return luaL_driver_error(L, error);
    }

    lua_pushlstring(L, value, strlen(value));
    free(value);

    return 1;
}

static int llora_get_DevEui(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char *value;

    driver_error_t *error = lora_mac_get(LORA_MAC_GET_DEVEUI, &value);
    if (error) {
    	return luaL_driver_error(L, error);
    }

    lua_pushlstring(L, value, strlen(value));
    free(value);

    return 1;
}

static int llora_get_AppEui(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char *value;

    driver_error_t *error = lora_mac_get(LORA_MAC_GET_APPEUI, &value);
    if (error) {
    	return luaL_driver_error(L, error);
    }

    lua_pushlstring(L, value, strlen(value));
    free(value);

    return 1;
}

static int llora_get_Dr(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char *value;

    driver_error_t *error = lora_mac_get(LORA_MAC_GET_DR, &value);
    if (error) {
    	return luaL_driver_error(L, error);
    }

    lua_pushinteger(L, atoi(value));
    free(value);

    return 1;
}

static int llora_get_Adr(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char *value;

    driver_error_t *error = lora_mac_get(LORA_MAC_GET_ADR, &value);
    if (error) {
    	return luaL_driver_error(L, error);
    }

    if (strcmp(value,"on") == 0) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }

    free(value);

    return 1;
}

static int llora_get_ReTx(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	char *value;

    driver_error_t *error = lora_mac_get(LORA_MAC_GET_RETX, &value);
    if (error) {
    	return luaL_driver_error(L, error);
    }

    lua_pushinteger(L, atoi(value));
    free(value);

    return 1;
}

static int llora_join(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	driver_error_t *error = lora_join();
    if (error) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

static int llora_tx(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	luaL_checktype(L, 1, LUA_TBOOLEAN);
    int cnf = lua_toboolean( L, 1 );
    int port = luaL_checkinteger(L, 2);
    const char *data = luaL_checkstring(L, 3);

    if ((port < 1) || (port > 223)) {
        return luaL_error(L, "%d:invalid port number", LORA_ERR_INVALID_ARGUMENT);
    }

    if (!lcheck_hex_str(data)) {
        luaL_error(L, "%d:invalid data", LORA_ERR_INVALID_ARGUMENT);
    }

    driver_error_t *error = lora_tx(cnf, port, data);
    if (error) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

static int llora_rx(lua_State* L) {
	if (is_gateway) luaL_exception_extended(L, LORA_ERR_NOT_ALLOWED, "only allowed for nodes");

	luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushvalue(L, 1);

    rx_callback = luaL_ref(L, LUA_REGISTRYINDEX);

    rx_callbackL = L;
    lora_set_rx_callback(on_received);

    return 0;
}
#endif

static const LUA_REG_TYPE lora_map[] = {
    { LSTRKEY( "attach" ),       LFUNCVAL( llora_attach ) },
#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276 || CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272
	{ LSTRKEY( "setDevAddr" ),   LFUNCVAL( llora_set_setDevAddr ) },
    { LSTRKEY( "setDevEui" ),    LFUNCVAL( llora_set_DevEui ) },
    { LSTRKEY( "setAppEui" ),    LFUNCVAL( llora_set_AppEui ) },
    { LSTRKEY( "setAppKey" ),    LFUNCVAL( llora_set_AppKey ) },
    { LSTRKEY( "setNwksKey" ),   LFUNCVAL( llora_set_NwkSKey ) },
    { LSTRKEY( "setAppsKey" ),   LFUNCVAL( llora_set_AppSKey ) },
    { LSTRKEY( "setDr" ),        LFUNCVAL( llora_set_Dr ) },
    { LSTRKEY( "setAdr" ),       LFUNCVAL( llora_set_Adr ) },
    { LSTRKEY( "setReTx" ),      LFUNCVAL( llora_set_ReTx ) },
    { LSTRKEY( "getDevAddr" ),   LFUNCVAL( llora_get_DevAddr ) },
    { LSTRKEY( "getDevEui" ),    LFUNCVAL( llora_get_DevEui ) },
    { LSTRKEY( "getAppEui" ),    LFUNCVAL( llora_get_AppEui ) },
    { LSTRKEY( "getDr" ),        LFUNCVAL( llora_get_Dr ) },
    { LSTRKEY( "getAdr" ),       LFUNCVAL( llora_get_Adr ) },
    { LSTRKEY( "getReTx" ),      LFUNCVAL( llora_get_ReTx ) },
    { LSTRKEY( "join" ),         LFUNCVAL( llora_join ) },
    { LSTRKEY( "tx" ),           LFUNCVAL( llora_tx ) },
    { LSTRKEY( "whenReceived" ), LFUNCVAL( llora_rx ) },

	// Constant definitions
    { LSTRKEY( "BAND868" ),		 LINTVAL( 868 ) },
    { LSTRKEY( "BAND433" ), 	 LINTVAL( 433 ) },
    { LSTRKEY( "BAND915" ), 	 LINTVAL( 915 ) },

    { LSTRKEY( "NODE"     ), 	 LINTVAL( 0 ) },
    { LSTRKEY( "GATEWAY"  ), 	 LINTVAL( 1 ) },

	DRIVER_REGISTER_LUA_ERRORS(lora)
#endif

	{LNILKEY, LNILVAL}
};

int luaopen_lora(lua_State* L) {
	return 0;
}

MODULE_REGISTER_ROM(LORA, lora, lora_map, luaopen_lora, 1);

#endif

/*
	Simple channel gateway example:

 	net.wf.setup(net.wf.mode.STA,"CITILAB","wifi@citilab")
	net.wf.start()
	net.service.sntp.start()

	lora.attach(lora.BAND868, lora.GATEWAY, nil, nil, 868100000, 5)

 */

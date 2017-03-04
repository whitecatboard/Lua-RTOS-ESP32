/*
 * Lua RTOS, Lora WAN Lua Module
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
#include "lualib.h"
#include "lauxlib.h"
#include "modules.h"
#include "error.h"

#if LUA_USE_LORA

#include <string.h>
#include <stdlib.h>
#include <drivers/lora.h>
#include <drivers/uart.h>

static int rx_callback = 0;
static lua_State* rx_callbackL;

extern const LUA_REG_TYPE lora_error_map[];

static void on_received(int port, char *payload) {
    if (rx_callback != LUA_NOREF) {
        lua_rawgeti(rx_callbackL, LUA_REGISTRYINDEX, rx_callback);
        lua_pushinteger(rx_callbackL, port);
        lua_pushlstring(rx_callbackL, payload, strlen(payload));
        lua_call(rx_callbackL, 2, 0);
    }
    
    free(payload);
}

// Checks if passed strings represents a valid hex number
static int check_hex_str(const char *str) {
    while (*str) {
        if (((*str < '0') || (*str > '9')) && ((*str < 'A') || (*str > 'F'))) {
            return 0;
        }

        str++;
    }
    
    return 1;
}

// Pads a hex number string representation at a specified length
static char *hex_str_pad(lua_State* L, const char  *str, int len) {
    if (!check_hex_str(str)) {
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

static int llora_setup(lua_State* L) {    
	driver_error_t *error;
    
    int band = luaL_checkinteger(L, 1);

    // Sanity checks
    if ((band != 868) && (band != 433)) {
        return luaL_error(L, "%d:invalid band", LORA_ERR_INVALID_ARGUMENT);
    }
    
    // Setup in base of frequency
    error = lora_setup(band);
    if (error) {
        return luaL_driver_error(L, error);
    }
	
    return 0;
}

static int llora_set_setDevAddr(lua_State* L) {
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
    driver_error_t *error = lora_join();
    if (error) {
        return luaL_driver_error(L, error);
    }
    
    return 0;
}

static int llora_tx(lua_State* L) {
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    int cnf = lua_toboolean( L, 1 );
    int port = luaL_checkinteger(L, 2);
    const char *data = luaL_checkstring(L, 3);
    
    if ((port < 1) || (port > 223)) {
        return luaL_error(L, "%d:invalid port number", LORA_ERR_INVALID_ARGUMENT);
    }

    if (!check_hex_str(data)) {
        luaL_error(L, "%d:invalid data", LORA_ERR_INVALID_ARGUMENT);
    }    
    
    driver_error_t *error = lora_tx(cnf, port, data);
    if (error) {
        return luaL_driver_error(L, error);
    }
    
    return 0;    
}

static int llora_rx(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushvalue(L, 1); 

    rx_callback = luaL_ref(L, LUA_REGISTRYINDEX);
            
    rx_callbackL = L;
    lora_set_rx_callback(on_received);
    
    return 0;
}

static const LUA_REG_TYPE lora_map[] = {
    { LSTRKEY( "setup" ),        LFUNCVAL( llora_setup ) }, 
    { LSTRKEY( "setDevAddr" ),   LFUNCVAL( llora_set_setDevAddr ) }, 
    { LSTRKEY( "setDevEui" ),    LFUNCVAL( llora_set_DevEui ) }, 
    { LSTRKEY( "setAppEui" ),    LFUNCVAL( llora_set_AppEui ) }, 
    { LSTRKEY( "setAppKey" ),    LFUNCVAL( llora_set_AppKey ) }, 
    { LSTRKEY( "setNwksKey" ),   LFUNCVAL( llora_set_NwkSKey ) }, 
    { LSTRKEY( "setAppsKey" ),   LFUNCVAL( llora_set_AppSKey ) }, 
    { LSTRKEY( "setAppKey" ),    LFUNCVAL( llora_set_AppKey ) }, 
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

	// Error definitions
	{LSTRKEY("error"), 			 LROVAL( lora_error_map )},

	{LNILKEY, LNILVAL}
};

int luaopen_lora(lua_State* L) {
#if !LUA_USE_ROTABLE
	luaL_newlib(L, lora_map);
	return 1;
#else
	return 0;
#endif		   
}

MODULE_REGISTER_MAPPED(LORA, lora, lora_map, luaopen_lora);

#endif

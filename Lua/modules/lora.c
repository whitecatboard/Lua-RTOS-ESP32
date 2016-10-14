/*
 * Whitecat, Lora WAN Lua Module
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

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "modules.h"
#include "error.h"

#if LUA_USE_LORA

#include <string.h>
#include <stdlib.h>
#include <sys/drivers/error.h>
#include <sys/drivers/lora.h>
#include <sys/drivers/uart.h>

static int rx_callback = 0;
static lua_State* rx_callbackL;

static void on_received(int port, char *payload) {
    if (rx_callback != LUA_NOREF) {
        lua_rawgeti(rx_callbackL, LUA_REGISTRYINDEX, rx_callback);
        lua_pushinteger(rx_callbackL, port);
        lua_pushlstring(rx_callbackL, payload, strlen(payload));
        lua_call(rx_callbackL, 2, 0);
    }
    
    free(payload);
}

static void lora_error(lua_State* L, int code) {
	uart0_default();
	
    switch (code){
        case LORA_KEYS_NOT_CONFIGURED:
            luaL_error(L, "%d:keys are not configured", LORA_KEYS_NOT_CONFIGURED);break;
        case LORA_ALL_CHANNELS_BUSY:
            luaL_error(L, "%d:all channels are busy", LORA_ALL_CHANNELS_BUSY);break;
        case LORA_DEVICE_IN_SILENT_STATE:
            luaL_error(L, "%d:device is in silent state", LORA_DEVICE_IN_SILENT_STATE);break;
        case LORA_DEVICE_DEVICE_IS_NOT_IDLE:
            luaL_error(L, "%d:device is not idle", LORA_DEVICE_DEVICE_IS_NOT_IDLE);break;
        case LORA_PAUSED:
            luaL_error(L, "%d:lora stack are paused", LORA_PAUSED);break;
        case LORA_TIMEOUT:
            luaL_error(L, "%d:time out", LORA_TIMEOUT);break;
        case LORA_JOIN_DENIED:
            luaL_error(L, "%d:join denied", LORA_JOIN_DENIED);break;
        case LORA_UNEXPECTED_RESPONSE:
            luaL_error(L, "%d:unexpected response", LORA_UNEXPECTED_RESPONSE);break;
        case LORA_NOT_JOINED:
            luaL_error(L, "%d:not joined", LORA_NOT_JOINED);break;
        case LORA_REJOIN_NEEDED:
            luaL_error(L, "%d:rejoin needed", LORA_REJOIN_NEEDED);break;
        case LORA_INVALID_DATA_LEN:
            luaL_error(L, "%d:invalid data len", LORA_INVALID_DATA_LEN);break;
        case LORA_TRANSMISSION_FAIL_ACK_NOT_RECEIVED:
            luaL_error(L, "%d:transmission fail, ack not received", LORA_TRANSMISSION_FAIL_ACK_NOT_RECEIVED);break;
        case LORA_NOT_SETUP:
            luaL_error(L, "%d:lora is not setup, setup first", LORA_NOT_SETUP);break;
        case LORA_INVALID_PARAM:
            luaL_error(L, "%d:invalid argument", LORA_INVALID_ARGUMENT);break;
        case LORA_NO_MEM:
            luaL_error(L, "%d:not enough memory", LORA_NO_MEM);break;
    }
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
    tdriver_error *error;
    
    int band = luaL_optinteger(L, 1, 868);
    
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    
    int rx_listener = lua_toboolean( L, 2 );

    // Sanity checks
    if ((band != 868) && (band != 433)) {
        return luaL_error(L, "%d:invalid band", LORA_INVALID_ARGUMENT);
    }
    
    // Setup in base of frequency
    error = lora_setup(band, rx_listener);
    if (error) {
		uart0_default();
        return luaL_driver_error(L, "lora can't setup", error);
    }
	
    return 0;
}

static int llora_set_setDevAddr(lua_State* L) {
    char *devAddr = hex_str_pad(L, luaL_checkstring(L, 1), 8);
    
	uart0_swap();
    int resp = lora_mac_set("devaddr", devAddr);
    if (resp != LORA_OK) {
        free(devAddr);
        lora_error(L, resp);    
    }
    uart0_default();
    free(devAddr);
    return 0;    
}

static int llora_set_DevEui(lua_State* L) {
    char  *devEui = hex_str_pad(L, luaL_checkstring(L, 1), 16);
    
	uart0_swap();
    int resp = lora_mac_set("deveui", devEui);
    if (resp != LORA_OK) {
        free(devEui);
        lora_error(L, resp);    
    }
    uart0_default();
	
    free(devEui);
    return 0;  
}

static int llora_set_AppEui(lua_State* L) {
    char  *appEui = hex_str_pad(L, luaL_checkstring(L, 1), 16);
        
	uart0_swap();
    int resp = lora_mac_set("appeui", appEui);
    if (resp != LORA_OK) {
        free(appEui);
        lora_error(L, resp);    
    }
	uart0_default();
	
    free(appEui);
    return 0;  
}

static int llora_set_NwkSKey(lua_State* L) {
    char  *nwkSKey = hex_str_pad(L, luaL_checkstring(L, 1), 32);
        
	uart0_swap();
    int resp = lora_mac_set("nwkskey", nwkSKey);
    if (resp != LORA_OK) {
        free(nwkSKey);
        lora_error(L, resp);    
    }
    uart0_default();
	
    free(nwkSKey);
    return 0;  
}

static int llora_set_AppSKey(lua_State* L) {
    char  *appSKey = hex_str_pad(L, luaL_checkstring(L, 1), 32);
        
	uart0_swap();
    int resp = lora_mac_set("appsKey", appSKey);
    if (resp != LORA_OK) {
        free(appSKey);
        lora_error(L, resp);    
    }
	uart0_default();
	
    free(appSKey);
    return 0;
}

static int llora_set_AppKey(lua_State* L) {
    char  *appKey = hex_str_pad(L, luaL_checkstring(L, 1), 32);
        
	uart0_swap();
    int resp = lora_mac_set("appkey", appKey);
    if (resp != LORA_OK) {
        free(appKey);
        lora_error(L, resp);    
    }
	uart0_default();

    free(appKey);
    return 0;
}

static int llora_set_Dr(lua_State* L) {
    int dr = luaL_checkinteger(L, 1);
    
    if ((dr < 0) || (dr > 7)) {
        return luaL_error(L, "%d:invalid data rate value (0 to 7)", LORA_INVALID_ARGUMENT); 
    }
    
    char value[2];
    
    sprintf(value,"%d", dr);
        
	uart0_swap();
    int resp = lora_mac_set("dr", value);
    if (resp != LORA_OK) {
        lora_error(L, resp);    
    }
	uart0_default();
	
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
    
	uart0_swap();
    int resp = lora_mac_set("adr", value);
    if (resp != LORA_OK) {
        lora_error(L, resp);    
    }
	uart0_default();

    return 0;
}

static int llora_set_RetX(lua_State* L) {
    int rets = luaL_checkinteger(L, 1);
    
    if ((rets < 0) || (rets > 255)) {
        return luaL_error(L, "%d:invalid retransmissions value (0 to 255)", LORA_INVALID_ARGUMENT); 
    }
    
    char value[2];
    
    sprintf(value,"%d", rets);
        
	uart0_swap();
    int resp = lora_mac_set("retx", value);
    if (resp != LORA_OK) {
        lora_error(L, resp);    
    }
	uart0_default();
	
    return 0;
}

static int llora_set_Ar(lua_State* L) {
    char value[4];

    luaL_checktype(L, 1, LUA_TBOOLEAN);
    if (lua_toboolean( L, 1 )) {
        strcpy(value, "on");
    } else {
        strcpy(value, "off");
    }
    
	uart0_swap();
    int resp = lora_mac_set("ar", value);
    if (resp != LORA_OK) {
        lora_error(L, resp);    
    }
	uart0_default();
    
    return 0;
}


static int llora_set_LinkChk(lua_State* L) {
    int interval = luaL_checkinteger(L, 1);
    
    if ((interval < 0) || (interval > 65535)) {
        return luaL_error(L, "%d:invalid interval (0 to 65535)", LORA_INVALID_ARGUMENT); 
    }
    
    char value[6];
    
    sprintf(value,"%d", interval);
        
	uart0_swap();
    int resp = lora_mac_set("linkchk", value);
    if (resp != LORA_OK) {
        lora_error(L, resp);    
    }
	uart0_default();

    return 0;
}

static int llora_get_DevAddr(lua_State* L) {
	uart0_swap();
    char *value = lora_mac_get("devaddr");
    uart0_default();
	
    lua_pushlstring(L, value, strlen(value));
    free(value);
    
    return 1;    
}

static int llora_get_DevEui(lua_State* L) {
	uart0_swap();
    char *value = lora_mac_get("deveui");
    uart0_default();
	
    lua_pushlstring(L, value, strlen(value));
    free(value);
    
    return 1;    
}

static int llora_get_AppEui(lua_State* L) {
	uart0_swap();
    char *value = lora_mac_get("appeui");
    uart0_default();
	
    lua_pushlstring(L, value, strlen(value));
    free(value);
    
    return 1;    
}

static int llora_get_Dr(lua_State* L) {
	uart0_swap();
    char *value = lora_mac_get("dr");
    uart0_default();
	
    lua_pushinteger(L, atoi(value));
    free(value);
    
    return 1;    
}

static int llora_get_Adr(lua_State* L) {
	uart0_swap();
    char *value = lora_mac_get("adr");
    uart0_default();
	
    if (strcmp(value,"on") == 0) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);        
    }

    free(value);
    
    return 1;    
}

static int llora_get_RetX(lua_State* L) {
	uart0_swap();
    char *value = lora_mac_get("retx");
    uart0_default();
	
    lua_pushinteger(L, atoi(value));
    free(value);
    
    return 1;    
}

static int llora_get_Ar(lua_State* L) {
	uart0_swap();
    char *value = lora_mac_get("ar");
    uart0_default();
	
    if (strcmp(value,"on") == 0) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);        
    }

    free(value);
    
    return 1;    
}

static int llora_get_Mrgn(lua_State* L) {
	uart0_swap();
    char *value = lora_mac_get("mrgn");
    uart0_default();
	
    lua_pushinteger(L, atoi(value));
    free(value);
    
    return 1;    
}

static int llora_join(lua_State* L) {
    int resp = 0;
    
    int join_type = luaL_checkinteger(L, 1);

    if ((join_type != 1) && (join_type != 2)) {
        return luaL_error(L, "%d:invalid join type, user lora.OTAA or lora.ABP", LORA_INVALID_ARGUMENT);                
    }
    
    if (join_type == 2) {
        return luaL_error(L, "%d:ABP not allowed", LORA_INVALID_ARGUMENT);                
    }
    
	uart0_swap();
    if (join_type == 1)
        resp = lora_join_otaa();
    
    if (resp != LORA_OK) {
        lora_error(L, resp);
    }
	uart0_default();
    
    return 0;
}

static int llora_tx(lua_State* L) {
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    int cnf = lua_toboolean( L, 1 );
    int port = luaL_checkinteger(L, 2);
    const char *data = luaL_checkstring(L, 3);
    
    if ((port < 1) || (port > 223)) {
        return luaL_error(L, "%d:invalid port number", LORA_INVALID_ARGUMENT);   
    }

    if (!check_hex_str(data)) {
        luaL_error(L, "%d:invalid data", LORA_INVALID_ARGUMENT);     
    }    
    
	uart0_swap();
    int resp = lora_tx(cnf, port, data);
    if (resp != LORA_OK) {
        lora_error(L, resp);
    }
	uart0_default();
    
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

static int llora_nothing(lua_State* L) {
    return luaL_error(L, "%d:not implemented", LORA_INVALID_ARGUMENT);    
}

static const LUA_REG_TYPE lora_error_map[] = {
	{ LSTRKEY( "KeysNotConfigured" ),	 LINTVAL( LORA_KEYS_NOT_CONFIGURED ) },
	{ LSTRKEY( "AllChannelsBusy" ),		 LINTVAL( LORA_ALL_CHANNELS_BUSY ) },
	{ LSTRKEY( "DeviceInSilentState" ),	 LINTVAL( LORA_DEVICE_IN_SILENT_STATE ) },
	{ LSTRKEY( "DeviceIsNotIdle" ),		 LINTVAL( LORA_DEVICE_DEVICE_IS_NOT_IDLE ) },
	{ LSTRKEY( "Paused" ),		         LINTVAL( LORA_PAUSED ) },
	{ LSTRKEY( "Timeout" ),		         LINTVAL( LORA_TIMEOUT ) },
	{ LSTRKEY( "JoinDenied" ),		     LINTVAL( LORA_JOIN_DENIED ) },
	{ LSTRKEY( "UnexpectedResponse" ),	 LINTVAL( LORA_UNEXPECTED_RESPONSE ) },
	{ LSTRKEY( "NotJoined" ),		     LINTVAL( LORA_NOT_JOINED ) },
	{ LSTRKEY( "RejoinNeeded" ),		 LINTVAL( LORA_REJOIN_NEEDED ) },
	{ LSTRKEY( "InvalidDataLen" ),		 LINTVAL( LORA_INVALID_DATA_LEN ) },
	{ LSTRKEY( "TransmissionFail" ),	 LINTVAL( LORA_TRANSMISSION_FAIL_ACK_NOT_RECEIVED ) },
	{ LSTRKEY( "NotSetup" ),		     LINTVAL( LORA_NOT_SETUP ) },
	{ LSTRKEY( "InvalidArgument" ),		 LINTVAL( LORA_INVALID_PARAM ) },
};

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
    { LSTRKEY( "setRetX" ),      LFUNCVAL( llora_set_RetX ) }, 
    { LSTRKEY( "setLinkChk" ),   LFUNCVAL( llora_set_LinkChk ) }, // MUST DO
    { LSTRKEY( "setRxDelay1" ),  LFUNCVAL( llora_nothing ) }, 
    { LSTRKEY( "setAr" ),        LFUNCVAL( llora_set_Ar ) }, 
    { LSTRKEY( "setRx2" ),       LFUNCVAL( llora_nothing ) }, // MUST DO
    { LSTRKEY( "setChFreq" ),    LFUNCVAL( llora_nothing ) }, // MUST DO
    { LSTRKEY( "setChFCycle" ),  LFUNCVAL( llora_nothing ) }, 
    { LSTRKEY( "setChDrrange" ), LFUNCVAL( llora_nothing ) }, // MUST DO
    { LSTRKEY( "setChStatus" ),  LFUNCVAL( llora_nothing ) }, 
    { LSTRKEY( "getDevAddr" ),   LFUNCVAL( llora_get_DevAddr ) }, 
    { LSTRKEY( "getDevEui" ),    LFUNCVAL( llora_get_DevEui ) }, 
    { LSTRKEY( "getAppEui" ),    LFUNCVAL( llora_get_AppEui ) }, 
    { LSTRKEY( "getDr" ),        LFUNCVAL( llora_get_Dr ) }, 
    { LSTRKEY( "getAdr" ),       LFUNCVAL( llora_get_Adr ) }, 
    { LSTRKEY( "getRetX" ),      LFUNCVAL( llora_get_RetX ) }, 
    { LSTRKEY( "getRxDelay1" ),  LFUNCVAL( llora_nothing ) }, 
    { LSTRKEY( "getRxDelay2" ),  LFUNCVAL( llora_nothing ) },
    { LSTRKEY( "getAr" ),        LFUNCVAL( llora_get_Ar ) }, 
    { LSTRKEY( "getRx2" ),       LFUNCVAL( llora_nothing ) }, 
    { LSTRKEY( "getDCyclePs" ),  LFUNCVAL( llora_nothing ) }, 
    { LSTRKEY( "getMrgn" ),      LFUNCVAL( llora_get_Mrgn ) }, 
    { LSTRKEY( "getGwNb" ),      LFUNCVAL( llora_nothing ) }, 
    { LSTRKEY( "getStatus" ),    LFUNCVAL( llora_nothing ) }, 
    { LSTRKEY( "getCha" ),       LFUNCVAL( llora_nothing ) }, 
    { LSTRKEY( "join" ),         LFUNCVAL( llora_join ) }, 
    { LSTRKEY( "tx" ),           LFUNCVAL( llora_tx ) },
    { LSTRKEY( "whenReceived" ), LFUNCVAL( llora_rx ) },
	
	// Constant definitions
    { LSTRKEY( "BAND868" ),		 LINTVAL( 868 ) },
    { LSTRKEY( "BAND433" ), 	 LINTVAL( 433 ) },
    { LSTRKEY( "OTAA" ), 		 LINTVAL( 1 ) },
    { LSTRKEY( "ABP" ), 		 LINTVAL( 2 ) },

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

LUA_OS_MODULE(LORA, lora, lora_map);

#endif
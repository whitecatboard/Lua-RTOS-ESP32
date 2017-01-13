/*
 * Lua RTOS, Lua ADC module
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

#if LUA_USE_ADC

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"
#include "adc.h"
#include "modules.h"

#include <stdio.h>
#include <string.h>

#include <drivers/adc.h>

static int ladc_setup( lua_State* L ) {
    int id;
	driver_error_t *error;

    id = luaL_checkinteger( L, 1 );

    adc_userdata *adc = (adc_userdata *)lua_newuserdata(L, sizeof(adc_userdata));

    adc->adc = id;
    adc->ref_voltage = CPU_ADC_REF;
    
    if ((error = adc_setup())) {
    	return luaL_driver_error(L, error);
    }

    luaL_getmetatable(L, "adc");
    lua_setmetatable(L, -2);

    return 1;
}

static int ladc_setup_channel( lua_State* L ) {
    int channel, res;
    adc_userdata *adc = NULL;
	driver_error_t *error;

    adc = (adc_userdata *)luaL_checkudata(L, 1, "adc");
    luaL_argcheck(L, adc, 1, "adc expected");

    res = luaL_checkinteger( L, 2 );
    channel = luaL_checkinteger( L, 3 );

    if ((error = adc_setup_channel(channel))) {
    	return luaL_driver_error(L, error);
    }

    adc_userdata *nadc = (adc_userdata *)lua_newuserdata(L, sizeof(adc_userdata));
    memcpy(nadc, adc, sizeof(adc_userdata));
    
    switch (res) {
        case 6:  nadc->max_val = 63;  break;
        case 8:  nadc->max_val = 255; break;
        case 9:  nadc->max_val = 511; break;
        case 10: nadc->max_val = 1023;break;
        case 11: nadc->max_val = 2047;break;
        case 12: nadc->max_val = 4095;break;
    }

    nadc->resolution = res;
    nadc->chan = channel;

    luaL_getmetatable(L, "adc");
    lua_setmetatable(L, -2);

    return 1;
}

static int ladc_read( lua_State* L ) {
    int readed;
	driver_error_t *error;
    adc_userdata *adc = NULL;

    adc = (adc_userdata *)luaL_checkudata(L, 1, "adc");
    luaL_argcheck(L, adc, 1, "adc expected");

    if ((error = adc_read(adc->chan, &readed))) {
    	return luaL_driver_error(L, error);
    } else {
        // Normalize
        if (readed & (1 << (12 - adc->resolution - 1))) {
            readed = ((readed >> (12 - adc->resolution)) + 1) & adc->max_val;
        } else {
            readed =readed >> (12 - adc->resolution);
        }    

        lua_pushinteger( L, readed );
        lua_pushnumber( L, ((double)readed * (double)adc->ref_voltage) / (double)adc->max_val);
        return 2;
    }
}

static int ladc_index(lua_State *L);
static int ladc_chan_index(lua_State *L);

static const LUA_REG_TYPE ladc_error_map[] = {
};

static const LUA_REG_TYPE ladc_map[] = {
    { LSTRKEY( "setup"   ),			LFUNCVAL( ladc_setup   ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE ladc_chan_map[] = {
  	{ LSTRKEY( "read"      ),	 LFUNCVAL( ladc_read          ) },
  	{ LSTRKEY( "setupchan" ),	 LFUNCVAL( ladc_setup_channel ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE ladc_constants_map[] = {
	ADC_ADC0
	ADC_ADC1
	ADC_ADC_CH0
	ADC_ADC_CH1
	ADC_ADC_CH2
	ADC_ADC_CH3
	ADC_ADC_CH4
	ADC_ADC_CH5
	ADC_ADC_CH6
	ADC_ADC_CH7

	// Error definitions
	{LSTRKEY("error"),  LROVAL( ladc_error_map )},

	{ LNILKEY, LNILVAL }
};

static const luaL_Reg ladc_func[] = {
    { "__index", 	ladc_index },
    { NULL, NULL }
};

static const luaL_Reg ladc_chan_func[] = {
    { "__index", 	ladc_chan_index },
    { NULL, NULL }
};

static int ladc_index(lua_State *L) {
	const char *key = luaL_checkstring(L, 2);
	printf("index %s\r\n",key);

	return luaR_index(L, ladc_map, ladc_constants_map);
}

static int ladc_chan_index(lua_State *L) {
	return luaR_index(L, ladc_chan_map, NULL);
}

LUALIB_API int luaopen_adc( lua_State *L ) {
    luaL_newlib(L, ladc_func);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);

    luaL_newmetatable(L, "adc");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    luaL_setfuncs(L, ladc_chan_func, 0);
    lua_pop(L, 1);

    return 1;
}

LIB_INIT(ADC, adc, luaopen_adc);

#endif

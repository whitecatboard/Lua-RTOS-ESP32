/*
 * Lua RTOS, Lua ADC module
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

extern LUA_REG_TYPE adc_error_map[];

static int ladc_setup( lua_State* L ) {
    int id, res, channel, vref;
	driver_error_t *error;

    id = luaL_checkinteger( L, 1 );
    channel = luaL_checkinteger( L, 2 );
    res = luaL_checkinteger( L, 3 );
    vref = luaL_optinteger( L, 4, 3300 );

    adc_userdata *adc = (adc_userdata *)lua_newuserdata(L, sizeof(adc_userdata));

    adc->adc = id;
    adc->chan = channel;
    
    if ((error = adc_setup(id, channel, vref, res))) {
    	return luaL_driver_error(L, error);
    }

    luaL_getmetatable(L, "adc");
    lua_setmetatable(L, -2);

    return 1;
}

static int ladc_setup_channel( lua_State* L ) {
    return 0;
}

static int ladc_read( lua_State* L ) {
    int raw;
    double mvlots;
	driver_error_t *error;
    adc_userdata *adc = NULL;

    adc = (adc_userdata *)luaL_checkudata(L, 1, "adc");
    luaL_argcheck(L, adc, 1, "adc expected");

    if ((error = adc_read(adc->adc, adc->chan, &raw, &mvlots))) {
    	return luaL_driver_error(L, error);
    } else {
        lua_pushinteger( L, raw );
        lua_pushnumber( L, mvlots);
        return 2;
    }
}

static int ladc_index(lua_State *L);
static int ladc_chan_index(lua_State *L);

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
	ADC_ADC2
	ADC_ADC3
	ADC_ADC_CH0
	ADC_ADC_CH1
	ADC_ADC_CH2
	ADC_ADC_CH3
	ADC_ADC_CH4
	ADC_ADC_CH5
	ADC_ADC_CH6
	ADC_ADC_CH7
	{LSTRKEY("error"), 			  LROVAL( adc_error_map    )},
	{LSTRKEY("attenuation0db  "), LINTVAL( ADC_ATTEN_0db   )},
	{LSTRKEY("attenuation2_5db"), LINTVAL( ADC_ATTEN_2_5db )},
	{LSTRKEY("attenuation6db"  ), LINTVAL( ADC_ATTEN_6db   )},
	{LSTRKEY("attenuation11db" ), LINTVAL( ADC_ATTEN_11db  )},
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

MODULE_REGISTER_UNMAPPED(ADC, adc, luaopen_adc);

#endif

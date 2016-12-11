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

#include "modules.h"

static const LUA_REG_TYPE adc_method_map[] = {
  { LSTRKEY( "read"      ),	 LFUNCVAL( ladc_read          ) },
  { LSTRKEY( "setup"     ),	 LFUNCVAL( ladc_setup         ) },
  { LSTRKEY( "setupchan" ),	 LFUNCVAL( ladc_setup_channel ) },
  { LNILKEY, LNILVAL }
};

#if LUA_USE_ROTABLE

static const LUA_REG_TYPE adc_constants_map[] = {
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
	{ LNILKEY, LNILVAL }
};

static int luaL_adc_index(lua_State *L) {
	int res;

	if ((res = luaR_findfunction(L, adc_method_map)) != 0)
		return res;

	const char *key = luaL_checkstring(L, 2);
	const TValue *val = luaR_findentry(adc_constants_map, key, 0, NULL);
	if (val != luaO_nilobject) {
		lua_pushinteger(L, val->value_.i);
		return 1;
	}

	return (int)luaO_nilobject;
}

static const luaL_Reg adc_load_funcs[] = {
    { "__index"    , 	luaL_adc_index },
    { NULL, NULL }
};

static int luaL_madc_index(lua_State *L) {
  int fres;
  if ((fres = luaR_findfunction(L, adc_method_map)) != 0)
    return fres;

  return (int)luaO_nilobject;
}

static const luaL_Reg madc_load_funcs[] = {
    { "__index"    , 	luaL_madc_index },
    { NULL, NULL }
};

#else

static const luaL_Reg adc_map[] = {
	{ NULL, NULL }
};

#endif

LUALIB_API int luaopen_adc( lua_State *L ) {
#if !LUA_USE_ROTABLE
    int i;
    char buff[5];

    luaL_register( L, AUXLIB_ADC, adc_map );
    
    // Set it as its own metatable
    lua_pushvalue( L, -1 );
    lua_setmetatable( L, -2 );

    // create metatable for adc module
    luaL_newmetatable(L, "adc");
  
    // Module constants
    for(i=CPU_FIRST_ADC;i<=CPU_FIRST_ADC;i++) {
        sprintf(buff,"ADC%d",i);
        MOD_REG_INTEGER( L, buff, i );
    }

    for(i=CPU_FIRST_ADC_CH;i<=CPU_FIRST_ADC_CH;i++) {
        sprintf(buff,"ADC_CH%d",i);
        MOD_REG_INTEGER( L, buff, i );
    }

    // metatable.__index = metatable
    lua_pushliteral(L, "__index");
    lua_pushvalue(L,-2);
    lua_rawset(L,-3);
    
    // Setup the methods inside metatable
    luaL_register( L, NULL, adc_method_map );
#else
    luaL_newlib(L, adc_load_funcs);  /* new module */
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);

    luaL_newmetatable(L, "adc");  /* create metatable */
    lua_pushvalue(L, -1);  /* push metatable */
    lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */

    luaL_setfuncs(L, madc_load_funcs, 0);  /* add file methods to new metatable */
    lua_pop(L, 1);  /* pop new metatable */
#endif

    return 1;
}

#endif

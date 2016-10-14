/*
 * Whitecat, Lua ADC module
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

#include "whitecat.h"

#if LUA_USE_ADC

#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <Lua/modules/adc.h>

static int ladc_setup( lua_State* L ) {
    int id, ref_type, ref_voltage;

    id = luaL_checkinteger( L, 1 );
    ref_type = luaL_checkinteger( L, 2 );
    ref_voltage = luaL_checkinteger( L, 3 );

    adc_userdata *adc = (adc_userdata *)lua_newuserdata(L, sizeof(adc_userdata));

    adc->adc = id;
    adc->ref_type = ref_type;
    adc->ref_voltage = ref_voltage;
    
    // Setup adc module
    adc_setup(ref_type);
        
    luaL_getmetatable(L, "adc");
    lua_setmetatable(L, -2);

    return 1;
}

static int ladc_setup_channel( lua_State* L ) {
    int channel, res;
    int ret;
    adc_userdata *adc = NULL;

    adc = (adc_userdata *)luaL_checkudata(L, 1, "adc");
    luaL_argcheck(L, adc, 1, "adc expected");

    res = luaL_checkinteger( L, 2 );
    channel = luaL_checkinteger( L, 3 );
        
    if ((res != 12) && (res != 10) && (res != 8) && (res != 6)) {
        return luaL_error( L, "invalid resulution" );
    }
            
    // Setup channel
    if ((ret = adc_setup_channel(channel)) < 0) {
        switch (ret) {
            case ADC_NO_MEM:
                return luaL_error( L, "not enough memory" );
                
            case ADC_NOT_AVAILABLE_CHANNELS:
                return luaL_error( L, "no more channels are available" );
                
            case ADC_CHANNEL_DOES_NOT_EXIST:
                return luaL_error( L, "channel does not exist" );                
        }
    }

    adc_userdata *nadc = (adc_userdata *)lua_newuserdata(L, sizeof(adc_userdata));
    memcpy(nadc, adc, sizeof(adc_userdata));
    
    switch (res) {
        case 6:  nadc->max_val = 63;break;
        case 8:  nadc->max_val = 255;break;
        case 10: nadc->max_val = 1023;break;
        case 12: nadc->max_val = 4095;break;
    }

    nadc->resolution = res;
    nadc->chan = channel;

    luaL_getmetatable(L, "adc");
    lua_setmetatable(L, -2);

    return 1;
}

static int ladc_read( lua_State* L ) {
    int channel;
    int readed;
    adc_userdata *adc = NULL;

    adc = (adc_userdata *)luaL_checkudata(L, 1, "adc");
    luaL_argcheck(L, adc, 1, "adc expected");
    
    if ((readed = adc_read(adc->chan)) < 0) {
        return luaL_error( L, "channel is not setup" );
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
       
    return 0;
}

const luaL_Reg adc_method_map[] = {
  { "read", ladc_read },
  { "setup", ladc_setup },
  { "setupchan", ladc_setup_channel },
  { NULL, NULL }
};

const luaL_Reg adc_map[] = {
  { NULL, NULL }
};


LUALIB_API int luaopen_adc( lua_State *L ) {
    int i;
    char buff[5];

    luaL_register( L, AUXLIB_ADC, adc_map );
    
    // Set it as its own metatable
    lua_pushvalue( L, -1 );
    lua_setmetatable( L, -2 );

    // create metatable for adc module
    luaL_newmetatable(L, "adc");
  
    // Module constants  
    MOD_REG_INTEGER( L, "AVDD", 0 );

    for(i=0;i<=2;i++) {
        sprintf(buff,"ADC%d",i);
        MOD_REG_INTEGER( L, buff, i );
    }

    // metatable.__index = metatable
    lua_pushliteral(L, "__index");
    lua_pushvalue(L,-2);
    lua_rawset(L,-3);
    
    // Setup the methods inside metatable
    luaL_register( L, NULL, adc_method_map );
    
    return 1;
}

#endif
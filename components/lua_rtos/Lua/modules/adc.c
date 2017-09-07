/*
 * Lua RTOS, Lua ADC module
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_ADC

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"
#include "adc.h"
#include "modules.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <drivers/adc.h>
#include <drivers/adc_internal.h>
#include <drivers/cpu.h>

extern LUA_REG_TYPE adc_error_map[];

static int ladc_calib(lua_State* L) {
	driver_error_t *error;
	adc_channel_h_t ch;

    int id1 = luaL_checkinteger( L, 1 );
    int ch1 = luaL_checkinteger( L, 2 );
    int id2 = luaL_checkinteger( L, 3 );
    int ch2 = luaL_checkinteger( L, 4 );

    if (id1 != 1) {
    	return luaL_exception_extended(L, ADC_ERR_CANNOT_CALIBRATE, "only ADC1 can be calibrated");
    }

    if (id2 == 1) {
    	return luaL_exception_extended(L, ADC_ERR_CANNOT_CALIBRATE, "ADC1 cannot be used for calibrate");
    }

    // Route v_ref
    esp_err_t status = adc2_vref_to_gpio(ch1);
	if (status != ESP_OK){
		return luaL_exception_extended(L, ADC_ERR_CALIBRATION, "GPIO can be either 25, 26, 27");
	}

	// Set ADC for measuring v_ref
	if (id2 == CPU_LAST_ADC + 3) {
	    if ((error = adc_setup(id2, ch2, 0, 0, 1200, 0, &ch))) {
	    	return luaL_driver_error(L, error);
	    }
	} else {
	    if ((error = adc_setup(id2, ch2, 0, 0, 0, 0, &ch))) {
	    	return luaL_driver_error(L, error);
	    }
	}

	// Read v_ref
	double v_ref;

	if ((error = adc_read_avg(&ch, 1000, NULL, &v_ref))) {
		return luaL_driver_error(L, error);
	}

	lua_pushinteger(L, roundf((10 * v_ref) / 10));

	return 1;
}

static int ladc_attach( lua_State* L ) {
    int id, res, channel, vref, max;
	driver_error_t *error;

    id = luaL_checkinteger( L, 1 );
    channel = luaL_checkinteger( L, 2 );
    res = luaL_optinteger( L, 3, 0 );
    vref = luaL_optinteger( L, 4, 0 );
    max = luaL_optinteger( L, 5, 0 );

    adc_userdata *adc = (adc_userdata *)lua_newuserdata(L, sizeof(adc_userdata));

    if ((error = adc_setup(id, channel, 0, vref, max, res, &adc->h))) {
    	return luaL_driver_error(L, error);
    }

    luaL_getmetatable(L, "adc.chan");
    lua_setmetatable(L, -2);

    return 1;
}

static int ladc_read( lua_State* L ) {
    int raw;
    double mvlots;
	driver_error_t *error;
    adc_userdata *adc = NULL;

    adc = (adc_userdata *)luaL_checkudata(L, 1, "adc.chan");
    luaL_argcheck(L, adc, 1, "adc expected");

    if ((error = adc_read(&adc->h, &raw, &mvlots))) {
    	return luaL_driver_error(L, error);
    } else {
		lua_pushinteger(L, raw);
		lua_pushnumber(L, mvlots);
        return 2;
    }
}

static const LUA_REG_TYPE ladc_map[] = {
	{ LSTRKEY( "calibrate"),	  LFUNCVAL( ladc_calib  ) },
    { LSTRKEY( "attach"),		  LFUNCVAL( ladc_attach  ) },
	ADC_ADC0
	ADC_ADC1
	ADC_ADC2
	ADC_ADC3
	ADC_ADC4
    { LSTRKEY( "MCP3008" ),		  LINTVAL( CPU_LAST_ADC + 1 ) },
    { LSTRKEY( "MCP3208" ),		  LINTVAL( CPU_LAST_ADC + 2 ) },
    { LSTRKEY( "ADS1115" ),		  LINTVAL( CPU_LAST_ADC + 3 ) },
	ADC_ADC_CH0
	ADC_ADC_CH1
	ADC_ADC_CH2
	ADC_ADC_CH3
	ADC_ADC_CH4
	ADC_ADC_CH5
	ADC_ADC_CH6
	ADC_ADC_CH7
	{LSTRKEY("error"), 			  LROVAL( adc_error_map    )},
	{ LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE ladc_chan_map[] = {
  	{ LSTRKEY( "read"        ),	  LFUNCVAL( ladc_read          ) },
    { LSTRKEY( "__metatable" ),	  LROVAL  ( ladc_chan_map      ) },
	{ LSTRKEY( "__index"     ),   LROVAL  ( ladc_chan_map      ) },
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_adc( lua_State *L ) {
    luaL_newmetarotable(L,"adc.chan", (void *)ladc_chan_map);
    return 0;
}

MODULE_REGISTER_MAPPED(ADC, adc, ladc_map, luaopen_adc);

#endif

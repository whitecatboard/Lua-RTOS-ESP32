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
 * Lua RTOS, Lua ADC module
 *
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
#if CONFIG_ADC_MCP3008
    { LSTRKEY( "MCP3008" ),		  LINTVAL( CPU_LAST_ADC + 1 ) },
#endif
#if CONFIG_ADC_MCP3208
	{ LSTRKEY( "MCP3208" ),		  LINTVAL( CPU_LAST_ADC + 1 ) },
#endif
#if CONFIG_ADC_ADS1115
    { LSTRKEY( "ADS1115" ),		  LINTVAL( CPU_LAST_ADC + 1 ) },
#endif
#if CONFIG_ADC_ADS1015
    { LSTRKEY( "ADS1015" ),		  LINTVAL( CPU_LAST_ADC + 1 ) },
#endif
	ADC_ADC_CH0
	ADC_ADC_CH1
	ADC_ADC_CH2
	ADC_ADC_CH3
	ADC_ADC_CH4
	ADC_ADC_CH5
	ADC_ADC_CH6
	ADC_ADC_CH7
	DRIVER_REGISTER_LUA_ERRORS(adc)
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

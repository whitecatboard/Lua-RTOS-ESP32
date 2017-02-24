/*
 * Lua RTOS, timer wrapper
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

#if LUA_USE_TMR

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"

#include <unistd.h>
#include <sys/delay.h>

static int tmr_delay( lua_State* L ) {
    unsigned long long period;

    period = luaL_checkinteger( L, 1 );
    
    delay(period * 1000);
    
    return 0;
}

static int tmr_delay_ms( lua_State* L ) {
    unsigned long long period;

    period = luaL_checkinteger( L, 1 );

    delay(period);
        
    return 0;
}

static int tmr_delay_us( lua_State* L ) {
    unsigned long long period;

    period = luaL_checkinteger( L, 1 );
    udelay(period);
    
    return 0;
}

static int tmr_sleep( lua_State* L ) {
    unsigned long long period;

    period = luaL_checkinteger( L, 1 );
    sleep(period);
    return 0;
}

static int tmr_sleep_ms( lua_State* L ) {
    unsigned long long period;

    period = luaL_checkinteger( L, 1 );
    usleep(period * 1000);
    return 0;
}

static int tmr_sleep_us( lua_State* L ) {
    unsigned long long period;

    period = luaL_checkinteger( L, 1 );
    usleep(period);
    
    return 0;
}

static const LUA_REG_TYPE tmr_map[] = {
    { LSTRKEY( "delay" ),			LFUNCVAL( tmr_delay ) },
    { LSTRKEY( "delayms" ),			LFUNCVAL( tmr_delay_ms ) },
    { LSTRKEY( "delayus" ),			LFUNCVAL( tmr_delay_us ) },
    { LSTRKEY( "sleep" ),			LFUNCVAL( tmr_sleep ) },
    { LSTRKEY( "sleepms" ),			LFUNCVAL( tmr_sleep_ms ) },
    { LSTRKEY( "sleepus" ),			LFUNCVAL( tmr_sleep_us ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_tmr( lua_State *L ) {
#if !LUA_USE_ROTABLE
    luaL_newlib(L, tmr_map);

    return 1;
#else
	return 0;
#endif
}

MODULE_REGISTER_MAPPED(TMR, tmr, tmr_map, luaopen_tmr);

#endif

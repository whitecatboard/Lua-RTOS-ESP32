/*
 * Lua RTOS, wifi module
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

#if LUA_USE_WIFI

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"
#include "wifi.h"
#include "error.h"

#include "modules.h"

static void lwifi_setup(lua_State* L) {
}

static void lwifi_start(lua_State* L) {
	return 0;
}

static void lwifi_stop(lua_State* L) {
	return 0;
}

static const LUA_REG_TYPE wifi_map[] = {
	{ LSTRKEY( "setup"     ),	 LFUNCVAL( lwifi_setup ) },
    { LSTRKEY( "start"     ),	 LFUNCVAL( lwifi_start ) },
    { LSTRKEY( "stop"      ),	 LFUNCVAL( lwifi_stop  ) },
#if LUA_USE_ROTABLE
//	{ LSTRKEY( "CONSOLE"  ),	 LINTVAL( CONSOLE_UART ) },
#endif
	{ LNILKEY, LNILVAL }
};

int luaopen_wifi(lua_State* L) {
#if !LUA_USE_ROTABLE
    luaL_newlib(L, wifi_map);

    int i;
    char buff[6];

    return 1;
#else
    return 0;
#endif
}

LUA_OS_MODULE(WIFI, wifi, wifi_map);

#endif

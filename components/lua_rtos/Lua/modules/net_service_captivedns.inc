/*
 * Lua RTOS, lua captivedns module
 *
 * Copyright (C) 2015 - 2017
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

#if LUA_USE_HTTP

#include "net.h"

#include "lwip/err.h"

#include <drivers/net.h>

extern void captivedns_start();
extern void captivedns_stop();

static int lcaptivedns_start(lua_State* L) {
	driver_error_t *error;

	if ((error = net_check_connectivity())) {
    	return luaL_driver_error(L, error);
	}

	captivedns_start();

	return 0;
}

static int lcaptivedns_stop(lua_State* L) {
	driver_error_t *error;

	if ((error = net_check_connectivity())) {
    	return luaL_driver_error(L, error);
	}

	captivedns_stop();

	return 0;
}

static const LUA_REG_TYPE captivedns_map[] = {
    { LSTRKEY( "start" ),	 LFUNCVAL( lcaptivedns_start   ) },
    { LSTRKEY( "stop"  ),	 LFUNCVAL( lcaptivedns_stop    ) },
	{ LNILKEY, LNILVAL }
};

#endif

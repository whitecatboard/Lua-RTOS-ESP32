/*
 * Lua RTOS, lua CAN module
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

#include "net.h"

#include <sys/driver.h>

#include <drivers/can.h>

static int lcan_service_start(lua_State* L) {
	driver_error_t *error;

	int id = luaL_checkinteger(L, 1);
	uint32_t speed = luaL_checkinteger(L, 2);
	uint32_t port = luaL_checkinteger(L, 3);

	if ((error = can_gateway_start(id, speed, port))) {
		return luaL_driver_error(L, error);
	}

	return 0;
}

static int lcan_service_stop(lua_State* L) {
	driver_error_t *error;

	if ((error = can_gateway_stop(0))) {
		return luaL_driver_error(L, error);
	}

	return 0;
}

static const LUA_REG_TYPE can_map[] = {
	{ LSTRKEY("start"), LFUNCVAL(lcan_service_start) },
	{ LSTRKEY("stop") , LFUNCVAL(lcan_service_stop ) },
	{ LNILKEY,LNILVAL }
};

/*
 * Lua RTOS, lua sntp module
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

#include "net.h"

#include "lwip/err.h"
#include "apps/sntp/sntp.h"

#include <drivers/net.h>

static int lsntp_start(lua_State* L) {
	driver_error_t *error;

	if ((error = net_check_connectivity())) {
    	return luaL_driver_error(L, error);
	}

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

	return 0;
}

static int lsntp_stop(lua_State* L) {
	driver_error_t *error;

	if ((error = net_check_connectivity())) {
    	return luaL_driver_error(L, error);
	}

	sntp_stop();

	return 0;
}

static const LUA_REG_TYPE sntp_map[] = {
    { LSTRKEY( "start" ),	 LFUNCVAL( lsntp_start   ) },
    { LSTRKEY( "stop"  ),	 LFUNCVAL( lsntp_stop    ) },
	{ LNILKEY, LNILVAL }
};

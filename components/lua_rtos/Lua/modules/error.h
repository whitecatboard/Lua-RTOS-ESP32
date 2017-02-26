/*
 * Lua RTOS, Lua helper functions for throw an error from a driver error
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

#ifndef _LUA_ERROR_H
#define	_LUA_ERROR_H

#include "lstate.h"

#include <sys/driver.h>

#define LUA_I2C_ID  I2C_DRIVER_ID
#define LUA_MQTT_ID 30

#define LUA_EXCEPTION_BASE(n) (n << 24)
#define LUA_EXCEPTION_CODE(module, n) (LUA_EXCEPTION_BASE(LUA_##module##_ID) | n)

int luaL_driver_error(lua_State* L, driver_error_t *error);

#define luaL_exception(l, exception) \
	luaL_error(l, \
		"%d:%s", \
		exception, \
		driver_get_err_msg_by_exception(exception) \
	)

#define luaL_exception_extended(l, exception, msg) \
	luaL_error(l, \
		"%d:%s (%s)", \
		exception, \
		driver_get_err_msg_by_exception(exception), \
		msg \
	)

#endif

/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, EDDYSTONE BEACON SERVICE Lua module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_BT

#include <sys/driver.h>
#include <sys/misc/hex_string.h>

typedef struct {
	int instance;
} leddystone_udata_t;

static uint8_t meta_created = 0;

static int leddystone_start(lua_State* L) {
	driver_error_t *error;

	leddystone_udata_t *udata = (leddystone_udata_t *) luaL_checkudata(L, 1,
	        "eddystone.ins");

	luaL_argcheck(L, udata, 1, "eddystone expected");

	if ((error = bt_eddystone_start(udata->instance))) {
		return luaL_driver_error(L, error);
	}

	return 0;
}

static int leddystone_stop(lua_State* L) {
	driver_error_t *error;

	leddystone_udata_t *udata = (leddystone_udata_t *) luaL_checkudata(L, 1,
	        "eddystone.ins");

	luaL_argcheck(L, udata, 1, "eddystone expected");

	if ((error = bt_eddystone_stop(udata->instance))) {
		return luaL_driver_error(L, error);
	}

	return 0;
}

static int eddystone_ins_gc(lua_State* L) {
	driver_error_t *error;

	leddystone_udata_t *udata = (leddystone_udata_t *) luaL_checkudata(L, 1,
	        "eddystone.ins");

	luaL_argcheck(L, udata, 1, "eddystone expected");

	if ((error = bt_eddystone_remove(udata->instance))) {
		return luaL_driver_error(L, error);
	}

	return 0;
}

static const LUA_REG_TYPE eddystone_ins_map[] = {
	{ LSTRKEY("start"        ), LFUNCVAL(leddystone_start     ) },
	{ LSTRKEY("stop"         ), LFUNCVAL(leddystone_stop      ) },
    { LSTRKEY( "__metatable" ),	LROVAL  ( eddystone_ins_map  ) },
	{ LSTRKEY( "__index"     ), LROVAL  ( eddystone_ins_map   ) },
	{ LSTRKEY( "__gc"        ), LFUNCVAL( eddystone_ins_gc    ) },
	{ LNILKEY,LNILVAL }
};

static int leddystone_uid(lua_State* L) {
	driver_error_t *error;

	// Get address
	const char *a_address = (const char *) luaL_checkstring(L, 1);
	if (!lcheck_hex_str(a_address)) {
		return luaL_exception_extended(L, BT_ERR_INVALID_ARGUMENT,
		        "address must be in hex string format");
	}

	// Get TX power
	bt_eddystone_tx_power_t tx_power = luaL_checkinteger(L, 2);

	// Get namespace
	const char *a_name_space = (const char *) luaL_checkstring(L, 3);
	if (!lcheck_hex_str(a_name_space)) {
		return luaL_exception_extended(L, BT_ERR_INVALID_ARGUMENT,
		        "namespace must be in hex string format");
	}

	// Get instance
	const char *a_instance = (const char *) luaL_checkstring(L, 4);
	if (!lcheck_hex_str(a_instance)) {
		return luaL_exception_extended(L, BT_ERR_INVALID_ARGUMENT,
		        "instance must be in hex string format");
	}

	if (!meta_created) {
		luaL_newmetarotable(L, "eddystone.ins", (void *) eddystone_ins_map);
		meta_created = 1;
	}

	// Create user data
	leddystone_udata_t *udata = (leddystone_udata_t *) lua_newuserdata(L,
	        sizeof(leddystone_udata_t));

	if (!udata) {
		return luaL_exception(L, BT_ERR_NOT_ENOUGH_MEMORY);
	}

	// Add
	bt_adress_t address;
	bt_eddystone_namespace_t name_space;
	bt_eddystone_instance_t instance;

	hex_string_to_val((char *) a_address, (char *) (address), 6, 0);
	hex_string_to_val((char *) a_name_space, (char *) (name_space), 10, 0);
	hex_string_to_val((char *) a_instance, (char *) (instance), 6, 0);

	error = bt_add_eddystone_uid(address, tx_power, name_space, instance,
	        &udata->instance);
	if (error) {
		free(udata);

		return luaL_driver_error(L, error);
	}

	luaL_getmetatable(L, "eddystone.ins");
	lua_setmetatable(L, -2);

	return 1;
}

static int leddystone_url(lua_State* L) {
	driver_error_t *error;

	// Get address
	const char *a_address = (const char *) luaL_checkstring(L, 1);
	if (!lcheck_hex_str(a_address)) {
		return luaL_exception_extended(L, BT_ERR_INVALID_ARGUMENT,
		        "address must be in hex string format");
	}

	// Get TX power
	bt_eddystone_tx_power_t tx_power = luaL_checkinteger(L, 2);

	// Get URL
	const char *url = (const char *) luaL_checkstring(L, 3);

	if (!meta_created) {
		luaL_newmetarotable(L, "eddystone.ins", (void *) eddystone_ins_map);
		meta_created = 1;
	}

	// Create user data
	leddystone_udata_t *udata = (leddystone_udata_t *) lua_newuserdata(L,
	        sizeof(leddystone_udata_t));

	if (!udata) {
		return luaL_exception(L, BT_ERR_NOT_ENOUGH_MEMORY);
	}

	// Add
	bt_adress_t address;

	hex_string_to_val((char *) a_address, (char *) (address), 6, 0);

	error = bt_add_eddystone_url(address, tx_power, url, &udata->instance);
	if (error) {
		return luaL_driver_error(L, error);
	}

	luaL_getmetatable(L, "eddystone.ins");
	lua_setmetatable(L, -2);

	return 1;
}

static const LUA_REG_TYPE eddystone_map[] = {
	{ LSTRKEY("uid"), LFUNCVAL(leddystone_uid) },
	{ LSTRKEY("url"), LFUNCVAL(leddystone_url) },
	{ LNILKEY, LNILVAL } };

#endif

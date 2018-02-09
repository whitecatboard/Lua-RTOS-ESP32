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
 * Lua RTOS, Lua bluetooth module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_BT

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"
#include "error.h"
#include "hex.h"

#include <string.h>

#include <drivers/bluetooth.h>

#include "bluetooth_eddystone.inc"

static int lbt_attach( lua_State* L ) {
	driver_error_t *error;

	int mode = luaL_checkinteger( L, 1 );

    if ((error = bt_setup(mode))) {
    	return luaL_driver_error(L, error);
    }

	return 0;
}

static int lbt_reset( lua_State* L ) {
	driver_error_t *error;

    if ((error = bt_reset())) {
    	return luaL_driver_error(L, error);
    }

	return 0;
}

static int lbt_advertise( lua_State* L ) {
	driver_error_t *error;
	bte_advertise_params_t params;

	params.interval_min = luaL_checkinteger( L, 1 ) / 0.625;
	params.interval_max = luaL_checkinteger( L, 2 ) / 0.625;

	params.type = luaL_checkinteger( L, 3 );
	params.own_address_type = luaL_checkinteger( L, 4 );
	params.peer_address_type = luaL_checkinteger( L, 5 );

	const char *peer_addr = luaL_checkstring(L, 6);

    if (!lcheck_hex_str(peer_addr)) {
    	return luaL_exception_extended(L, BT_ERR_INVALID_ARGUMENT, "peer address must be in hex string format");
    }

    memset(params.peer_address, 0, 6);
    hex_string_to_val((char *)peer_addr, (char *)(params.peer_address), 6, 0);

	params.chann_map = luaL_checkinteger( L, 7 );
	params.filter_policy = luaL_checkinteger( L, 8 );

	const char *adv_data = luaL_checkstring(L, 9);
    if (!lcheck_hex_str(adv_data)) {
    	return luaL_exception_extended(L, BT_ERR_INVALID_ARGUMENT, "advertise data must be in hex string format");
    }

    uint8_t *data = calloc(1, strlen(adv_data) / 2);
    if (!data) {
    	return luaL_exception(L, BT_ERR_NOT_ENOUGH_MEMORY);
    }

    hex_string_to_val((char *)adv_data, (char *)(data), strlen(adv_data) / 2, 0);

    if ((error = bt_adv_start(params, data, strlen(adv_data)))) {
    	free(data);

    	return luaL_driver_error(L, error);
    }

    free(data);

	return 0;
}

static const LUA_REG_TYPE lbt_service[] = {
	{ LSTRKEY( "eddystone" ), LROVAL ( eddystone_map ) },
};

static const LUA_REG_TYPE lbt_mode[] = {
	{ LSTRKEY( "Idle"       ), LINTVAL( ((bt_mode_t)Idle)    ) },
	{ LSTRKEY( "BLE"        ), LINTVAL( ((bt_mode_t)BLE)     ) },
	{ LSTRKEY( "Classic"    ), LINTVAL( ((bt_mode_t)Classic) ) },
	{ LSTRKEY( "Dual"       ), LINTVAL( ((bt_mode_t)Dual)    ) },
};

static const LUA_REG_TYPE lbt_adv_type[] = {
	{ LSTRKEY( "ADV_IND"         	 ), LINTVAL( ADV_IND	         ) },
	{ LSTRKEY( "ADV_DIRECT_IND_HIGH" ), LINTVAL( ADV_DIRECT_IND_HIGH ) },
	{ LSTRKEY( "ADV_DIRECT_IND_LOW"  ), LINTVAL( ADV_DIRECT_IND_LOW  ) },
	{ LSTRKEY( "ADV_NONCONN_IND" 	 ), LINTVAL( ADV_NONCONN_IND 	 ) },
	{ LSTRKEY( "ADV_SCAN_IND"    	 ), LINTVAL( ADV_SCAN_IND	     ) },
};

static const LUA_REG_TYPE lbt_own_addr_type[] = {
	{ LSTRKEY( "Public"          ), LINTVAL( OwnPublic        ) },
	{ LSTRKEY( "Random"  		 ), LINTVAL( OwnRandom        ) },
	{ LSTRKEY( "PrivatePublic"   ), LINTVAL( OwnPrivatePublic ) },
	{ LSTRKEY( "PrivateRandom"   ), LINTVAL( OwnPrivateRandom ) },
};

static const LUA_REG_TYPE lbt_peer_addr_type[] = {
	{ LSTRKEY( "Public"          ), LINTVAL( PeerPublic       ) },
	{ LSTRKEY( "Random"  		 ), LINTVAL( PeerRandom       ) },
};

static const LUA_REG_TYPE lbt_adv_channel_map[] = {
	{ LSTRKEY( "37"              ), LINTVAL( Chann37       ) },
	{ LSTRKEY( "38"              ), LINTVAL( Chann38       ) },
	{ LSTRKEY( "39"              ), LINTVAL( Chann39       ) },
	{ LSTRKEY( "All"             ), LINTVAL( AllChann      ) },
};

static const LUA_REG_TYPE lbt_adv_filter_policy[] = {
	{ LSTRKEY( "ConnAllScanAll"     ), LINTVAL( ConnAllScanAll     ) },
	{ LSTRKEY( "ConnAllScanWhite"   ), LINTVAL( ConnAllScanWhite   ) },
	{ LSTRKEY( "ConnWhiteScanAll"   ), LINTVAL( ConnWhiteScanAll   ) },
	{ LSTRKEY( "ConnWhiteScanWhite" ), LINTVAL( ConnWhiteScanWhite ) },
};

static const LUA_REG_TYPE lbt_map[] = {
	{ LSTRKEY( "attach"     ), LFUNCVAL( lbt_attach            ) },
	{ LSTRKEY( "reset"      ), LFUNCVAL( lbt_reset             ) },
	{ LSTRKEY( "advertise"  ), LFUNCVAL( lbt_advertise         ) },
	{ LSTRKEY( "mode"       ), LROVAL  ( lbt_mode              ) },
	{ LSTRKEY( "adv"        ), LROVAL  ( lbt_adv_type          ) },
	{ LSTRKEY( "ownaddr"    ), LROVAL  ( lbt_own_addr_type     ) },
	{ LSTRKEY( "peeraddr"   ), LROVAL  ( lbt_peer_addr_type    ) },
	{ LSTRKEY( "chann"      ), LROVAL  ( lbt_adv_channel_map   ) },
	{ LSTRKEY( "filter"     ), LROVAL  ( lbt_adv_filter_policy ) },
	{ LSTRKEY( "service"    ), LROVAL  ( lbt_service ) },
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_bt( lua_State *L ) {
	return 0;
}

MODULE_REGISTER_MAPPED(BT, bt, lbt_map, luaopen_bt);

#endif

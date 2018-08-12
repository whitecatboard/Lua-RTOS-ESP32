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
 * Lua RTOS, Lua CAN module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_CAN

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "error.h"
#include "can.h"
#include "modules.h"

#include <signal.h>

#include <can_bus.h>

#include <drivers/can.h>
#include <drivers/cpu.h>

static uint8_t dump_stop = 0;

extern CAN_stats_t CAN_stat;

static int lcan_attach(lua_State* L) {
	driver_error_t *error;

	int id = luaL_checkinteger(L, 1);
	uint32_t speed = luaL_checkinteger(L, 2);
	uint16_t rx_size = luaL_optinteger(L, 3, 200);

    if ((error = can_setup(id, speed, rx_size))) {
    	return luaL_driver_error(L, error);
    }

    return 0;
}

static int lcan_send(lua_State* L) {
	driver_error_t *error;

	int id = luaL_checkinteger(L, 1);
	int msg_id = luaL_checkinteger(L, 2);
	int msg_id_type = luaL_checkinteger(L, 3);
	int len = luaL_checkinteger(L, 4);
        size_t dlen;
	const char *data = luaL_checklstring(L, 5,&dlen);

        if ( dlen < len )
		len = dlen;
    if ((error = can_tx(id, msg_id, msg_id_type, (uint8_t *)data, len))) {
    	return luaL_driver_error(L, error);
    }

	return 0;
}

static int lcan_add_filter(lua_State* L) {
	driver_error_t *error;

	int id = luaL_checkinteger(L, 1);
	int fromId = luaL_checkinteger(L, 2);
	int toId = luaL_checkinteger(L, 3);

    if ((error = can_add_filter(id, fromId, toId))) {
    	return luaL_driver_error(L, error);
    }

	return 0;
}

static int lcan_remove_filter(lua_State* L) {
	driver_error_t *error;

	int id = luaL_checkinteger(L, 1);
	int fromId = luaL_checkinteger(L, 2);
	int toId = luaL_checkinteger(L, 3);

    if ((error = can_remove_filter(id, fromId, toId))) {
    		return luaL_driver_error(L, error);
    }

	return 0;
}

static int lcan_recv(lua_State* L) {
	driver_error_t *error;
	uint32_t msg_id;
	uint8_t msg_id_type;
	uint8_t len;
	uint8_t data[9] = {0,0,0,0,0,0,0,0,0};

	int id = luaL_checkinteger(L, 1);

    if ((error = can_rx(id, &msg_id, &msg_id_type, data, &len))) {
    		return luaL_driver_error(L, error);
    }

	lua_pushinteger(L, msg_id);
	lua_pushinteger(L, msg_id_type);
	lua_pushinteger(L, len);
	lua_pushlstring(L, (const char *) data, (size_t) len);

	return 4;
}

static void ldump_stop (int i) {
	dump_stop = 1;
}

static int lcan_dump(lua_State* L) {
	driver_error_t *error;
	uint32_t msg_id;
	uint8_t msg_id_type;
	uint8_t len;
	uint8_t data[9] = {0,0,0,0,0,0,0,0,0};
	int id = luaL_checkinteger(L, 1);
	int i;

	dump_stop = 0;
	signal(SIGINT, ldump_stop);

	while (!dump_stop) {
	    if ((error = can_rx(id, &msg_id, &msg_id_type, data, &len))) {
	    		return luaL_driver_error(L, error);
	    }

	    printf("can%d  %08x  [%d] ", id, msg_id, len);

	    for(i = 0;i<len;i++) {
	    	if (i > 0) {
	    		printf(" ");
	    	}

	    	printf("%02x", data[i]);
	    }

	    printf("\r\n");
	}

	return 0;
}

static int lcan_stats(lua_State* L) {
	uint8_t table = 0;

	// Check if user wants result as a table, or wants scan's result
	// on the console
	if (lua_gettop(L) == 1) {
		luaL_checktype(L, 1, LUA_TBOOLEAN);
		if (lua_toboolean(L, 1)) {
			table = 1;
		}
	}

	if (table) {
		lua_createtable(L, 0, 12);

		lua_pushinteger(L, CAN_stat.rx.packets);
		lua_setfield (L, -2, "rx_packets");

		lua_pushinteger(L, CAN_stat.rx.bytes);
		lua_setfield (L, -2, "rx_bytes");

		lua_pushinteger(L, CAN_stat.tx.packets);
		lua_setfield (L, -2, "tx_packets");

		lua_pushinteger(L, CAN_stat.hw_overruns);
		lua_setfield (L, -2, "hw_overruns");

		lua_pushinteger(L, CAN_stat.sw_overruns);
		lua_setfield (L, -2, "sw_overruns");

		lua_pushinteger(L, CAN_stat.bus_errors);
		lua_setfield (L, -2, "bus_error");

		lua_pushinteger(L, CAN_stat.arbitration_lost_errors);
		lua_setfield (L, -2, "arbitration_lost_errors");

		lua_pushinteger(L, CAN_stat.passive_errors);
		lua_setfield (L, -2, "passive_errors");

		lua_pushinteger(L, CAN_stat.irq_errors);
		lua_setfield (L, -2, "irq_errors");
	} else {
		printf("RX packets:%d bytes:%d\r\n", CAN_stat.rx.packets, CAN_stat.rx.bytes);
		printf("TX packets:%d bytes:%d\r\n", CAN_stat.tx.packets, CAN_stat.tx.bytes);
		printf("hw overruns:%d\r\n",CAN_stat.hw_overruns);
		printf("sw overruns:%d\r\n",CAN_stat.sw_overruns);

		printf("errors:\r\n");
		printf("   hw overruns:%d\r\n",CAN_stat.hw_overruns);
		printf("   sw overruns:%d\r\n",CAN_stat.sw_overruns);
		printf("   bus:%d arbitration lost:%d passive:%d irq:%d\r\n",
			CAN_stat.bus_errors,
			CAN_stat.arbitration_lost_errors,
			CAN_stat.passive_errors,
			CAN_stat.irq_errors
		);
	}

	return table;
}

static const LUA_REG_TYPE lcan_map[] = {
    { LSTRKEY( "attach"       ),		  LFUNCVAL( lcan_attach        ) },
    { LSTRKEY( "addfilter"    ),		  LFUNCVAL( lcan_add_filter    ) },
    { LSTRKEY( "removefilter" ),		  LFUNCVAL( lcan_remove_filter ) },
    { LSTRKEY( "send"         ),		  LFUNCVAL( lcan_send          ) },
    { LSTRKEY( "receive"      ),		  LFUNCVAL( lcan_recv          ) },
    { LSTRKEY( "dump"         ),		  LFUNCVAL( lcan_dump          ) },
    { LSTRKEY( "stats"        ),		  LFUNCVAL( lcan_stats         ) },
	CAN_CAN0
	CAN_CAN1
	DRIVER_REGISTER_LUA_ERRORS(can)
	{LSTRKEY("STD"), LINTVAL(0)},
	{LSTRKEY("EXT"), LINTVAL(1)},

	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_can( lua_State *L ) {
    return 0;
}

MODULE_REGISTER_ROM(CAN, can, lcan_map, luaopen_can, 1);

#endif


/*

can.setup(can.CAN0, 1000)
while true do
    frame = string.pack(
    	"BBBBBBBB",
    	math.random(255), math.random(255), math.random(255), math.random(255),
    	math.random(255), math.random(255), math.random(255), math.random(255)
    )

	can.send(can.CAN0, 100, can.STD, 8, frame)
	tmr.delayms(50)
end

can.setup(can.CAN0, 1000)
can.dump(can.CAN0)

 */

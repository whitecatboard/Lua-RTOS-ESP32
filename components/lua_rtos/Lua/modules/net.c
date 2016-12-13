#include "luartos.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"
#include "net.h"
#include "error.h"

#include "modules.h"

#include "net_wifi.inc"


static int lnet_stat(lua_State* L) {
	u8_t table = 0;

	// Check if user wants result as a table, or wants scan's result
	// on the console
	if (lua_gettop(L) == 1) {
		luaL_checktype(L, 1, LUA_TBOOLEAN);
		if (lua_toboolean(L, 1)) {
			table = 1;
		}
	}

	if (table) {
		lua_createtable(L, 1, 0);

		lua_pushinteger(L, 0);
	}

	// This should be done in a more elegant way in future versions ...

	// Call wf.stat
	lua_getglobal(L, "net");
	lua_getfield(L, -1, "wf");
	lua_getfield(L, -1, "stat");
    lua_pushboolean(L, table);
    lua_pcall(L, 1, 1, 0);

    if (table) {
    	lua_remove(L, 4);
    	lua_remove(L, 4);
    	lua_settable(L,-3);
    } else {
    	lua_settop(L, 0);
    }

	return table;
}

static const LUA_REG_TYPE net_map[] = {
	{ LSTRKEY( "stat"  ),	 LFUNCVAL( lnet_stat   ) },
#if LUA_USE_ROTABLE
	{ LSTRKEY( "wf"    ),	 LROVAL   ( wifi_map   ) },
#endif
	{ LNILKEY, LNILVAL }
};

int luaopen_net(lua_State* L) {
#if !LUA_USE_ROTABLE
    luaL_newlib(L, net_map);

    return 1;
#else
    return 0;
#endif
}

LUA_OS_MODULE(NET, net, net_map);

/*

net.wf.setup(net.wf.mode.STA, "CITILAB","wifi@citilab")
net.wf.start()
a = net.stat(true)
net.wf.stat()
net.wf.stat(true)

 */

/*
 * Copyright (C) 2015 - 2018, Thomas E. Horner (whitecatboard.org@horner.it)
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
 * Lua RTOS, Lua captivedns net module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_HTTP_SERVER

#include "net.h"

#include "lwip/err.h"

#include <drivers/net.h>

extern int captivedns_start(lua_State* L);
extern void captivedns_stop();
extern int captivedns_running();

static int lcaptivedns_start(lua_State* L) {
	if (!wait_for_network(20000)) {
		luaL_exception(L, NET_ERR_NOT_AVAILABLE);
	}

	return captivedns_start(L);
}

static int lcaptivedns_stop(lua_State* L) {
	captivedns_stop();
	return 0;
}

static int lcaptivedns_running( lua_State* L ) {
	lua_pushboolean(L, captivedns_running());
	return 1;
}

static const LUA_REG_TYPE captivedns_map[] = {
	{ LSTRKEY( "start"  ),	 LFUNCVAL( lcaptivedns_start   ) },
	{ LSTRKEY( "stop"   ),	 LFUNCVAL( lcaptivedns_stop    ) },
	{ LSTRKEY( "running"),	 LFUNCVAL( lcaptivedns_running ) },
	{ LNILKEY, LNILVAL }
};

#endif

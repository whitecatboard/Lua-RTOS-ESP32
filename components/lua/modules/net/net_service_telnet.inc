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
 * Lua RTOS, Lua telnet net module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_TELNET_SERVER

#include "net.h"

#include "lwip/err.h"

#include <drivers/net.h>

extern int telnet_start(lua_State* L);
extern void telnet_stop();
extern int telnet_print(lua_State* L);

static u8_t _telnet_running = 0;

static int ltelnet_start(lua_State* L) {
	telnet_start(L);
	_telnet_running = 1;
	return 0;
}

static int ltelnet_stop(lua_State* L) {
	telnet_stop();
	_telnet_running = 0;
	return 0;
}

static int ltelnet_print(lua_State* L) {
	return telnet_print(L);
}

static int ltelnet_running( lua_State* L ) {
	lua_pushboolean(L, _telnet_running);
	return 1;
}

static const LUA_REG_TYPE telnet_map[] = {
	{ LSTRKEY( "start"   ),	 LFUNCVAL( ltelnet_start   ) },
	{ LSTRKEY( "stop"    ),	 LFUNCVAL( ltelnet_stop    ) },
	{ LSTRKEY( "running" ),	 LFUNCVAL( ltelnet_running ) },
	{ LSTRKEY( "print"   ),	 LFUNCVAL( ltelnet_print   ) },
	{ LNILKEY, LNILVAL }
};

#endif

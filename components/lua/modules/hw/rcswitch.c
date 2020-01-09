/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2019, Thomas E. Horner (whitecatboard.org@horner.it)
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
 * Lua RTOS, Lua RCSwitch module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_RCSWITCH

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "error.h"
#include "modules.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/status.h>
#include <sys/delay.h>
#include "sys.h"
#include <sys/sleep.h>
#include <rc-switch.h>

static int lrcswitch_send(lua_State *L) {
  return rcswitch_send(L);
}

static int lrcswitch_receive(lua_State *L) {
  return rcswitch_listen(L);
}

static int lrcswitch_stop(lua_State *L) {
  rcswitch_destroy(L);
  return 0;
}

static const LUA_REG_TYPE lrcswitch_map[] = {
  { LSTRKEY( "send" ),                   LFUNCVAL( lrcswitch_send    ) },
  { LSTRKEY( "receive" ),                LFUNCVAL( lrcswitch_receive ) },
  { LSTRKEY( "stop" ),                   LFUNCVAL( lrcswitch_stop    ) },

  { LSTRKEY( "BACKGROUND" ),             LINTVAL( 0 ) },
  DRIVER_REGISTER_LUA_ERRORS(rcswitch)
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_rcswitch( lua_State *L ) {
#if !LUA_USE_ROTABLE
  luaL_newlib(L, rcswitch);
  return 1;
#else
	return 0;
#endif
}

MODULE_REGISTER_ROM(RCSWITCH, rcswitch, lrcswitch_map, luaopen_rcswitch, 1);

#endif

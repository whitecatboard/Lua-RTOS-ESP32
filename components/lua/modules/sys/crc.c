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
 * Lua RTOS, Lua crc module
 *
 */

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"

#if CONFIG_LUA_RTOS_LUA_USE_CRC

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <rom/crc.h>

static int lcrc8(lua_State *L) {
	size_t length;
	const uint8_t *string = (uint8_t *) luaL_checklstring(L, 1, &length);
	uint8_t crc = luaL_optinteger(L, 2, 0xFF );
	crc = crc8_le(crc, string, length);

	lua_pushinteger(L, (lua_Integer)crc);
	return 1;
}

static int lcrc16(lua_State *L) {
	size_t length;
	const uint8_t *string = (uint8_t *) luaL_checklstring(L, 1, &length);
	uint16_t crc = luaL_optinteger(L, 2, 0xFFFF );
	crc = crc16_le(crc, string, length);

	lua_pushinteger(L, (lua_Integer)crc);
	return 1;
}

static int lcrc32(lua_State *L) {
	size_t length;
	const uint8_t *string = (uint8_t *) luaL_checklstring(L, 1, &length);
	uint32_t crc = luaL_optinteger(L, 2, 0xFFFFFFFF );
	crc = crc32_le(crc, string, length);

	lua_pushinteger(L, (lua_Integer)crc);
	return 1;
}

static const LUA_REG_TYPE crc_map[] = 
{
  { LSTRKEY( "crc8"   ),    LFUNCVAL( lcrc8   ) },
  { LSTRKEY( "crc16"   ),    LFUNCVAL( lcrc16   ) },
  { LSTRKEY( "crc32"   ),    LFUNCVAL( lcrc32   ) },
  { LNILKEY, LNILVAL }
};

int luaopen_crc(lua_State *L) {
	#if !LUA_USE_ROTABLE
	luaL_newlib(L, crc_map);
	return 1;
	#else
	return 0;
	#endif		   
}
	   
MODULE_REGISTER_ROM(CRC, crc, crc_map, luaopen_crc, 1);

#endif

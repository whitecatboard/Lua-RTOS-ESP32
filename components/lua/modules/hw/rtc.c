/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, RTC module
 *
 */

#include "sdkconfig.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "error.h"
#include "modules.h"

#include <string.h>

#include <sys/mutex.h>

#include <sys/drivers/rtc.h>


typedef enum {
	RTCMetaNilType = 1,
	RTCMetaIntegerType = 2,
	RTCMetaNumberType = 3,
	RTCMetaBooleanTrueType = 4,
	RTCMetaBooleanFalseType = 5,
} rtc_meta_type_t;

static int lpush(lua_State *L) {
	driver_error_t *error;

	// Get element meta type, and its value
    rtc_meta_type_t meta;
    uint8_t *val;

    lua_Integer luaIntegerVal;
    lua_Number luaNumberVal;

	int top = 1;

    switch(lua_type(L, top)) {
    	case LUA_TNIL:
        	meta = RTCMetaNilType;
        	break;

        case LUA_TNUMBER:
            if (lua_isinteger(L,top)) {
            	meta = RTCMetaIntegerType;
            	luaIntegerVal = luaL_checkinteger(L, top);
            	val = (uint8_t *)&luaIntegerVal;
            } else {
            	meta = RTCMetaNumberType;
            	luaNumberVal = luaL_checknumber(L, top);
            	val = (uint8_t *)&luaNumberVal;
            }
            break;

        case LUA_TBOOLEAN:
            if (lua_toboolean(L, top)) {
            	meta = RTCMetaBooleanTrueType;
            } else {
            	meta = RTCMetaBooleanFalseType;
            }
            break;

        default:
        	return luaL_exception(L, RTC_ERR_TYPE_NOT_ALLOWED);
    }

    if ((error = rtc_mem_push(meta, val))) {
    	return luaL_driver_error(L, error);
    }

	return 0;
}

static int lpop(lua_State *L) {
	driver_error_t *error;
	uint8_t meta;
	size_t size;
	void *val;

    if ((error = rtc_mem_pop(&meta, &size, &val))) {
    	return luaL_driver_error(L, error);
    }

    switch (meta) {
    	case RTCMetaNilType:
    		lua_pushnil(L);
    		break;
    	case RTCMetaIntegerType:
    		lua_pushinteger(L, *((lua_Integer *)val));
    		free(val);
    		break;
    	case RTCMetaNumberType:
    		lua_pushnumber(L, *((lua_Number *)val));
    		free(val);
    		break;
    	case RTCMetaBooleanTrueType:
    		lua_pushboolean(L, 1);
    		break;
    	case RTCMetaBooleanFalseType:
    		lua_pushboolean(L, 0);
    		break;
    	default:
        	return luaL_exception(L, RTC_ERR_TYPE_NOT_ALLOWED);
    }

    return 1;
}

static int lsize(lua_State *L) {
    lua_pushinteger(L, rtc_mem_size());
    return 1;
}

static int lfree(lua_State *L) {
    lua_pushinteger(L, rtc_mem_free());
    return 1;
}

static const LUA_REG_TYPE lrtc_mem_map[] = {
  { LSTRKEY( "push" ),  LFUNCVAL( lpush      ) },
  { LSTRKEY( "pop"  ),  LFUNCVAL( lpop       ) },
  { LSTRKEY( "size" ),  LFUNCVAL( lsize      ) },
  { LSTRKEY( "free" ),  LFUNCVAL( lfree      ) },
  { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lrtc_map[] = {
  { LSTRKEY( "mem"  ),  LROVAL( lrtc_mem_map ) },
  DRIVER_REGISTER_LUA_ERRORS(rtc)
  { LNILKEY, LNILVAL }
};


LUALIB_API int luaopen_rtc( lua_State *L ) {
	// Register meta types
	rtc_mem_register_meta(RTCMetaNilType, 0);
	rtc_mem_register_meta(RTCMetaIntegerType, sizeof(lua_Integer));
	rtc_mem_register_meta(RTCMetaNumberType, sizeof(lua_Number));
	rtc_mem_register_meta(RTCMetaBooleanTrueType, 0);
	rtc_mem_register_meta(RTCMetaBooleanFalseType, 0);

	return 0;
}

MODULE_REGISTER_ROM(RTC, rtc, lrtc_map, luaopen_rtc, 1);

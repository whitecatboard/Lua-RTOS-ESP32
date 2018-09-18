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
 * Lua RTOS, Lua fs (file system) module
 *
 */

#include "sdkconfig.h"

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"

#include <errno.h>
#include <string.h>

#include <sys/mount.h>
#include <sys/driver.h>

#if CONFIG_LUA_RTOS_LUA_USE_FS

static int l_mount(lua_State *L) {
    const char *target = luaL_checkstring(L, 1);
    if (!target) {
    		return luaL_error(L, "target missing");
    }

    const char *fs = luaL_checkstring(L, 2);
    if (!fs) {
    		return luaL_error(L, "file system missing");
    }

    int ret;

    if ((ret = mount(target, fs) < 0)) {
    		if (errno == ENOENT) {
        		return luaL_error(L, "target is empty");
    		} else if (errno == ENODEV) {
        		return luaL_error(L, "non-existent file system");
    		} else if (errno == EBUSY) {
        		return luaL_error(L, "target is mounted");
    		} else if (errno == EINVAL) {
        		return luaL_error(L, "target is used by other file system");
    		} else if (errno == EPERM) {
        		return luaL_error(L, "not allowed");
    		} else {
    			return luaL_error(L, strerror(errno));
    		}
    }

    return 0;
}

static int l_umount(lua_State *L) {
    const char *target = luaL_checkstring(L, 1);
    if (!target) {
    		return luaL_error(L, "target missing");
    }
    
    int ret;

    if ((ret = umount(target)) < 0) {
		if (errno == ENOENT) {
			return luaL_error(L, "target is empty");
		} else if (errno == EINVAL) {
			return luaL_error(L, "target is not a mount point");
		} else if (errno == EPERM) {
			return luaL_error(L, "not allowed");
		} else {
			return luaL_error(L, strerror(errno));
		}
    }

    return 0;
}

static const LUA_REG_TYPE fs_map[] =
{
  { LSTRKEY( "mount" ),      LFUNCVAL( l_mount  ) },
  { LSTRKEY( "umount" ),     LFUNCVAL( l_umount ) },
  { LNILKEY, LNILVAL }
};

int luaopen_fs(lua_State *L) {
	return 0;
}
	   
MODULE_REGISTER_ROM(FS, fs, fs_map, luaopen_fs, 1);

#endif

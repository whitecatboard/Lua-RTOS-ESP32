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
 * Lua RTOS, Lua http net module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_HTTP_CLIENT

#include "net.h"

#include "lwip/err.h"

#include <drivers/net.h>
#include <drivers/net_http.h>

static int lhttp_get( lua_State* L ) {
    const char *server = luaL_checkstring(L, 1);
    const char *resource = luaL_checkstring(L, 2);
    const char *expected_content_type = luaL_optstring(L, 3, NULL);

    driver_error_t *error;
    net_http_client_t client = HTTP_CLIENT_INITIALIZER;
    net_http_response_t response;

    if ((error = net_http_create_client(server, "443", &client))) {
        return luaL_driver_error(L, error);
    }

    if ((error = net_http_get(&client, resource, expected_content_type, &response))) {
        net_http_destroy_client(&client);
        return luaL_driver_error(L, error);
    }

    // push response code
    lua_pushinteger(L, response.code);

    uint8_t count = 0;
    if ((response.code == 200) && (response.size > 0)) {

        count = 1;
        lua_createtable(L, count, 0); //initial table size 1

        uint8_t buffer[1024];
        while (response.size > 0){
            if ((error = net_http_read_response(&response, buffer, sizeof(buffer)))) {
                net_http_destroy_client(&client);
                return luaL_driver_error(L, error);
            }
            lua_pushlstring(L, (char*)buffer, response.len);
            lua_rawseti(L, -2, count++);
        }
    }

    if ((error = net_http_destroy_client(&client))) {
        return luaL_driver_error(L, error);
    }

    return (count>0) ? 2:1;
}

static const LUA_REG_TYPE http_client_map[] = {
    { LSTRKEY( "get"    ),           LFUNCVAL( lhttp_get       ) },
    { LNILKEY, LNILVAL }
};

#endif

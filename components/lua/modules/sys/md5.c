/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
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
 * Lua RTOS, Lua md5 module
 *
 */

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"

#if CONFIG_LUA_RTOS_LUA_USE_MD5

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <rom/md5_hash.h>
#include <sys/misc/hex_string.h>

#define DIGEST_VAL_LENGTH  16
#define DIGEST_HEX_LENGTH  2 * DIGEST_VAL_LENGTH

static int lmd5_string(lua_State *L) {
    size_t length;
    const uint8_t *string = (uint8_t *) luaL_checklstring(L, 1, &length);
    unsigned char digest[DIGEST_VAL_LENGTH];

    struct MD5Context ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, string, length);
    MD5Final(digest, &ctx);

    char digest_string[DIGEST_HEX_LENGTH];
    val_to_hex_string_caps(digest_string, (char *)digest, DIGEST_VAL_LENGTH, 0, 0, 0);

    lua_pushlstring(L, digest_string, DIGEST_HEX_LENGTH);
    return 1;
}

#define BUFFER_SIZE 64
static int lmd5_file(lua_State *L) {
    const char* binary = luaL_checkstring(L, 1);

    FILE *fp;
    fp = fopen(binary, "rb");
    if (!fp) {
        return luaL_fileresult(L, 0, binary);
    }

    struct MD5Context ctx;
    MD5Init(&ctx);

    uint8_t buffer[BUFFER_SIZE];
    size_t length;
    while((length = fread(buffer, 1, BUFFER_SIZE, fp))) {
        MD5Update(&ctx, buffer, length);
    }
    if (ferror(fp)) {
        int retval = luaL_fileresult(L, 0, binary);
        fclose(fp);
        return retval;
    }
    fclose(fp);

    unsigned char digest[DIGEST_VAL_LENGTH];
    MD5Final(digest, &ctx);

    char digest_string[DIGEST_HEX_LENGTH];
    val_to_hex_string_caps(digest_string, (char *)digest, DIGEST_VAL_LENGTH, 0, 0, 0);

    lua_pushlstring(L, digest_string, DIGEST_HEX_LENGTH);
    return 1;
}

static const LUA_REG_TYPE md5_map[] =
{
    { LSTRKEY( "file"   ),    LFUNCVAL( lmd5_file   ) },
    { LSTRKEY( "string"   ),  LFUNCVAL( lmd5_string   ) },
    { LNILKEY, LNILVAL }
};

int luaopen_md5(lua_State *L) {
    LNEWLIB(L, md5_map);
}

MODULE_REGISTER_ROM(MD5, md5, md5_map, luaopen_md5, 1);

#endif

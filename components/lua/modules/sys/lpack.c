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
 * Lua RTOS, Lua pack / unpack hex string module
 *
 */

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"

#if 0
#include "base64.h"
#endif

#include <stdlib.h>
#include <string.h>

#if CONFIG_LUA_RTOS_LUA_USE_PACK
#define PACK_NUMBER   0b0000
#define PACK_INTEGER  0b0001
#define PACK_NIL      0b0010
#define PACK_BOOLEAN  0b0011
#define PACK_STRING   0b0100

#define PACK_BITS_PER_TYPE    4
#define PACK_TYPES_PER_BYTE   (8 / PACK_BITS_PER_TYPE)
#define PACK_HEADER_LENGTH(n) \
    (sizeof(char) + (int)((n + (PACK_TYPES_PER_BYTE - 1)) / PACK_TYPES_PER_BYTE))

#define PACK_PACK_TYPE(t, n) \
(t << ((8 - PACK_BITS_PER_TYPE) - ((n-1) % PACK_TYPES_PER_BYTE)*PACK_BITS_PER_TYPE))
        
#define PACK_UNPACK_TYPE(v, n) \
((v & PACK_PACK_TYPE(0b1111, n)) >> ((8 - PACK_BITS_PER_TYPE) - ((n-1) % PACK_TYPES_PER_BYTE)*PACK_BITS_PER_TYPE))

// Convert an hex string buffer (hbuff argument) into a byte buffer (vbuff 
// argument) of len argument size
static void hex_string_to_val(char *hbuff, char *vbuff, int len) {
    int  i;
    char c;
    
    for(i=0;i<len;i++) {
        c = 0;
        if ((*hbuff >= '0') && (*hbuff <= '9')) {
            c = (0 + (*hbuff - '0')) << 4;
        }

        if ((*hbuff >= 'A') && (*hbuff <= 'F')) {
            c = (10 + (*hbuff - 'A')) << 4;
        }
        
        hbuff++;

        if ((*hbuff >= '0') && (*hbuff <= '9')) {
            c |= 0 + (*hbuff - '0');
        }

        if ((*hbuff >= 'A') && (*hbuff <= 'F')) {
            c |= 10 + (*hbuff - 'A');
        }
        
        *vbuff = c;

        hbuff++;
        vbuff++;
    }
}

// Convert byte buffer (vbuff argument) of len argument size into a hex
// string buffer (hbuff argument) into a ) 
static void val_to_hex_string(char *hbuff, char *vbuff, int len) {
    int i;

    for(i=0;i<len;i++) {
        if ((((*vbuff & 0xf0) >> 4) >= 0) && (((*vbuff & 0xf0) >> 4) <= 9)) {
            *hbuff = '0' + ((*vbuff & 0xf0) >> 4);
        }
        
        if ((((*vbuff & 0xf0) >> 4) >= 10) && (((*vbuff & 0xf0) >> 4) <= 15)) {
            *hbuff = 'A' + (((*vbuff & 0xf0) >> 4) - 10);
        }
        hbuff++;

        if (((*vbuff & 0x0f) >= 0) && ((*vbuff & 0x0f) <= 9)) {
            *hbuff = '0' + (*vbuff & 0x0f);
        }
        
        if (((*vbuff & 0x0f) >= 10) && ((*vbuff & 0x0f) <= 15)) {
            *hbuff = 'A' + ((*vbuff & 0x0f) - 10);
        }
        hbuff++;  
        vbuff++;
    }    
}

// Gets the length of an hex string coded in hbuff argument
static int hex_string_len(char *hbuff) {
    char c1;
    char c2;
    int len = 0;
    
    c1 = *hbuff++;
    c2 = *hbuff++;
    while ((c1 != '0') || (c2 != '0')) {
        len++;
        c1 = *hbuff++;
        c2 = *hbuff++;        
    }
    
    return len;
}

static int l_pack(lua_State *L) {
    int total = lua_gettop(L); // Number of arguments
    int i;                     // Current argument number
    char *pack;       // Packed string
    int data_idx;              // Current data index on pack string
    int header_len;            // Pack header length
    char *header;     // Header buffer
    char *cheader;    // Current position of header buffer

    // This variables are for store argument values
    char *luaStringVal;
    lua_Number luaNumberVal;
    lua_Integer luaIntegerVal;
    int luaBooleanVal;
    
    // Sanity checks
    if (total == 0) {
        return luaL_error(L, "missing arguments");
    }
    
    // Allocate space for header
    header_len = PACK_HEADER_LENGTH(total);
    header = (char *)malloc(header_len);
    if (!header) {
        return luaL_error(L, "not enough memory");
    }
    
    cheader = header;
    
    // Store number of packed elements in header
    *cheader = (char)total;

    // Get total argument size
    int argSize = 0;
    for(i=1; i <= total;i++) {
        switch(lua_type(L, i)) {
            case LUA_TNUMBER:
                if (lua_isinteger(L,i)) {
                    argSize += sizeof(lua_Integer);
                } else {
                    argSize += sizeof(lua_Number);                    
                }
                break;
                
            case LUA_TBOOLEAN:
                argSize += sizeof(char);
                break;

            case LUA_TSTRING:
                argSize += luaL_len(L, i) + 1;
                break;
        }
    }
    
    // Allocate space for pack string with enough space for encode the header,
    // encoded arguments and the end of string character
    pack = (char *)malloc((header_len + argSize) * 2 + 1);
    if (!pack) {
        free(header);
        return luaL_error(L, "not enough memory");
    }
    
    // Put index just before the end of header
    data_idx = header_len * 2;
    
    // Pack each element
    for(i=1; i <= total;i++) {
        if (((i-1) % PACK_TYPES_PER_BYTE) == 0) {
            cheader++;
            *cheader = 0;
        }
        
        switch(lua_type(L, i)) {
            case LUA_TNUMBER:
                if (lua_isinteger(L,i)) {
                    *cheader = *cheader | PACK_PACK_TYPE(PACK_INTEGER,i);                    

                    // Get value
                    luaIntegerVal = luaL_checkinteger(L, i);
                
                    // Encode value
                    val_to_hex_string(pack + data_idx, (char *)&luaIntegerVal, sizeof(lua_Integer));
                    data_idx = data_idx + (sizeof(lua_Integer) * 2);
                } else {
                    *cheader = *cheader | PACK_PACK_TYPE(PACK_NUMBER,i);                    
                    
                    // Get value
                    luaNumberVal = luaL_checknumber(L, i);
                
                    // Encode value
                    val_to_hex_string(pack + data_idx, (char *)&luaNumberVal, sizeof(lua_Number));
                    data_idx = data_idx + (sizeof(lua_Number) * 2);
                }
                
                *(pack + data_idx) = 0;                
                break;
                
            case LUA_TNIL:
                *cheader = *cheader | PACK_PACK_TYPE(PACK_NIL,i);
                *(pack + data_idx) = 0;                
                break;
                
            case LUA_TBOOLEAN:
                *cheader = *cheader | PACK_PACK_TYPE(PACK_BOOLEAN,i);

                // Get value
               luaBooleanVal = (char)lua_toboolean(L, i);
                 
                // Encode value
                val_to_hex_string(pack + data_idx, (char *)&luaBooleanVal, sizeof(char));
                data_idx = data_idx + (sizeof(char) * 2);
                *(pack + data_idx) = 0;                
                break;                

            case LUA_TSTRING:
                *cheader = *cheader | PACK_PACK_TYPE(PACK_STRING,i);

                // Get value
                luaStringVal = (char *)lua_tostring(L, i);
                
                // Encode value
                val_to_hex_string(pack + data_idx, luaStringVal, strlen(luaStringVal));
                data_idx = data_idx + (strlen(luaStringVal) * 2);
                *(pack + data_idx) = '0';data_idx++;
                *(pack + data_idx) = '0';data_idx++;
                *(pack + data_idx) = 0;
                break;  
            default:
                luaL_error(L, "unsupported type (argument %d)", i);break; 
        }
    }
    
    val_to_hex_string(pack, header, header_len);
    free(header);
    
    lua_pushstring(L, pack);
    free(pack);
    
    return 1;
}

#if 0
static int l_b64(lua_State *L) {
	char *hex;         // Data in hex string format
	uint8_t data[255]; // Data in binary data
	int size;          // Data size
	char b64[350];     // Data in base 64

	// Get data coded in hex string format
	hex = (char *)luaL_checkstring(L, 1);

	size = strlen(hex) / 2;

	// Convert data coded in hex string format to binary data
	hex_string_to_val((char *)hex, (char *)data, size);

	bin_to_b64(data, size, b64, sizeof(b64));

    lua_pushstring(L, b64);

    return 1;
}
#endif

static int l_unpack(lua_State *L) {
    int total;                   // Number of packet values
    int i;                       // Current packet value
    char *pack;         // Packed string
    int data_idx;                // Current data index on pack string
    char *header;       // Header buffer
    char *cheader;      // Current position of header buffer
    char headerv;       // Current header position buffer value
    char ctype;         // Current packet value type
    int slen;                    // For packet strings, it's length
    char unpack_first;  // If only first value must be unpack
    
    // This variables are for store argument values
    char *luaStringVal;
    lua_Number luaNumberVal;
    lua_Integer luaIntegerVal;
    char luaBooleanVal;

    if (lua_type(L, 1) == LUA_TNIL) {
        lua_pushnil(L);
        return 1;
    }
    
    // Get the packet string
    pack = (char *)luaL_checkstring(L, 1);

    // Check for an optional second argument, a boolean that tell if we want
    // to unpack only first value
    if (lua_type(L, 2) == LUA_TBOOLEAN) {
        unpack_first = lua_toboolean(L, 2);
    } else {
        unpack_first = 0;
    }

    // Init pointers
    header = pack;
    cheader = header;

    // Unpack number of arguments
    hex_string_to_val(cheader, &headerv, sizeof(char));
    total = headerv;

    // Put index just before the end of header
    data_idx = PACK_HEADER_LENGTH(total) * 2;

    // Unpack arguments ...
    for(i=1;i<=total;i++) {
        if (((i-1) % PACK_TYPES_PER_BYTE) == 0) {
            cheader += 2;
            
            // Unpack types
            hex_string_to_val(cheader, &headerv, sizeof(char));
        }
                
        ctype = PACK_UNPACK_TYPE(headerv, i);
        switch (ctype) {
            case PACK_NUMBER:
                // Unpack
                hex_string_to_val(pack + data_idx, (char *)&luaNumberVal, sizeof(lua_Number));
                data_idx += sizeof(lua_Number) * 2;                
                lua_pushnumber(L, luaNumberVal);
                break;
            case PACK_INTEGER:
                // Unpack
                hex_string_to_val(pack + data_idx, (char *)&luaIntegerVal, sizeof(lua_Integer));
                data_idx += sizeof(lua_Integer) * 2;
                lua_pushinteger(L, luaIntegerVal);
                break;
            case PACK_NIL:
                lua_pushnil(L);
                break;
            case PACK_BOOLEAN:
                // Unpack
                hex_string_to_val(pack + data_idx, (char *)&luaBooleanVal, sizeof(luaBooleanVal));
                data_idx += sizeof(luaBooleanVal) * 2;
                lua_pushboolean(L, luaBooleanVal);
                break;
            case PACK_STRING:
                // Get the string length
                slen = hex_string_len(pack + data_idx);
                
                // Allocate space for the string
                luaStringVal = (char *)malloc(slen + 1);
                if (!luaStringVal) {
                    return luaL_error(L, "not enough memory");
                }
                
                // Unpack
                hex_string_to_val(pack + data_idx, luaStringVal, slen);
                *(luaStringVal + slen) = 0;

                lua_pushstring(L, luaStringVal);
                
                free(luaStringVal);
                
                data_idx += slen * 2 + 2;
                break;
        }
    }
    
    if (unpack_first) {
        if (total > 1) {
            lua_getglobal(L, "pack"); 
            lua_getfield(L, -1, "pack");
            lua_remove(L, -2);
            lua_copy(L, 3, 1);
            lua_copy(L, lua_gettop(L), 3);
            lua_remove(L, -1);
            lua_pcall(L, total - 1, 2, 0); 
            lua_remove(L, -1);
            lua_remove(L, -2);
        } else {
            lua_pushnil(L);
        }
        return 2;
    } else {
        return total;        
    }

}

static const LUA_REG_TYPE pack_map[] = 
{
  { LSTRKEY( "pack"   ),    LFUNCVAL( l_pack   ) },
//  { LSTRKEY( "b64"    ),    LFUNCVAL( l_b64    ) },
  { LSTRKEY( "unpack" ),    LFUNCVAL( l_unpack ) },
  { LNILKEY, LNILVAL }
};

int luaopen_pack(lua_State *L) {
	#if !LUA_USE_ROTABLE
	luaL_newlib(L, pack_map);
	return 1;
	#else
	return 0;
	#endif		   
}
	   
MODULE_REGISTER_ROM(PACK, pack, pack_map, luaopen_pack, 1);

#endif

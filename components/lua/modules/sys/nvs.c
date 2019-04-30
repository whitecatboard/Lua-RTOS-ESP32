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
 * Lua RTOS, Lua nvs (non volatile storage) module
 *
 */

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#if CONFIG_LUA_RTOS_LUA_USE_NVS

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define LNVS_TYPE_INT     1
#define LNVS_TYPE_NUMBER  2
#define LNVS_TYPE_BOOLEAN 3
#define LNVS_TYPE_NIL     4
#define LNVS_TYPE_STRING  5

static void nvs_error(lua_State* L, int code) {
    switch (code){
        case ESP_FAIL:
            luaL_error(L, "%:dfail", ESP_FAIL);break;
        case ESP_ERR_INVALID_ARG:
            luaL_error(L, "%d:fail", ESP_ERR_INVALID_ARG);break;
        case ESP_ERR_NO_MEM:
            luaL_error(L, "%d:not enough memory", ESP_ERR_NO_MEM);break;
        case ESP_ERR_INVALID_STATE:
            luaL_error(L, "%d:invalid state", ESP_ERR_INVALID_STATE);break;
        case ESP_ERR_INVALID_SIZE:
            luaL_error(L, "%d:invalid size", ESP_ERR_INVALID_SIZE);break;
        case ESP_ERR_NVS_NOT_FOUND:
            luaL_error(L, "%d:key not found", ESP_ERR_NOT_FOUND);break;
    }
}

// Lua: nvs.write(namespace, key) return: nothing|exception
static int l_nvs_write(lua_State *L) {
    int total = lua_gettop(L); // Get number of arguments
    nvs_handle handle_to_settings;
    esp_err_t err;
    const char *key = NULL;
    const char *nspace = NULL;
    const char *str_val = NULL;
    size_t val_size = 0;
    void *val_val = NULL;

    // Sanity checks, and check arguments
    if (total != 3 ) {
    	return luaL_error(L, "missing arguments");
    }

    nspace = luaL_checkstring(L, 1);
    if (!nspace) {
    	return luaL_error(L, "namespace missing");
    }

    key = luaL_checkstring(L, 2);
    if (!key) {
    	return luaL_error(L, "key missing");
    }

    switch(lua_type(L, 3)) {
        case LUA_TNUMBER:
            if (lua_isinteger(L,3)) {
            	val_size = sizeof(lua_Integer) + 1;
            	val_val = (void *)malloc(val_size);
            	if (val_val) {
            		*((char*)val_val) = (char)LNVS_TYPE_INT;
            		*((lua_Integer *)(val_val + 1)) = luaL_checkinteger(L, 3);
            	}
            } else {
            	val_size = sizeof(lua_Number) + 1;
            	val_val = (void *)malloc(val_size);
            	if (val_val) {
            		*((char*)val_val) = (char)LNVS_TYPE_NUMBER;
            		*((lua_Number *)(val_val + 1)) = luaL_checknumber(L, 3);
            	}
            }

            break;

        case LUA_TBOOLEAN:
        	val_size = sizeof(int) + 1;
        	val_val = (void *)malloc(val_size);
        	if (val_val) {
        		*((char*)val_val) = (char)LNVS_TYPE_BOOLEAN;
        		*((int *)(val_val + 1)) = (char)lua_toboolean(L, 3);
        	}
            break;

        case LUA_TNIL:
        	val_size = sizeof(char) * 2;
        	val_val = (void *)malloc(val_size);
        	if (val_val) {
        		*((char*)val_val) = (char)LNVS_TYPE_NIL;
        		*((char *)(val_val + 1)) = 0;
        	}
            break;

        case LUA_TSTRING:
        	str_val = (char *)lua_tostring(L, 3);

        	val_size = strlen(str_val) + 2;
        	val_val = (void *)malloc(val_size);
        	if (val_val) {
        		*((char*)val_val) = (char)LNVS_TYPE_STRING;
        		memcpy((char *)(val_val + 1), str_val, val_size - 1);
        	}
            break;
    }

    // Open
    err = nvs_open(nspace, NVS_READWRITE, &handle_to_settings);
    if (err != ESP_OK) {
    	free(val_val);
    	nvs_error(L, err);
    }

    // Save
    err = nvs_set_blob(handle_to_settings, key, val_val, val_size);
    if (err != ESP_OK) {
    	free(val_val);
    	nvs_error(L, err);
    }

    free(val_val);

    // Commit changes
    err = nvs_commit(handle_to_settings);
    if (err != ESP_OK) {
    	nvs_error(L, err);
    }

    // Close
    nvs_close(handle_to_settings);

    return 0;
}

// Lua: nvs.rm(namespace, key) return: true|exception
static int l_nvs_read(lua_State *L) {
    int total = lua_gettop(L); // Get number of arguments
    nvs_handle handle_to_settings;
    esp_err_t err;
    const char *key = NULL;
    const char *nspace = NULL;
    int8_t val_type = 0;
    void *val_val = NULL;

    // Sanity checks, and check arguments
    if (total < 2 ) {
    	return luaL_error(L, "missing arguments");
    }

    nspace = luaL_checkstring(L, 1);
    if (!nspace) {
    	return luaL_error(L, "namespace missing");
    }

    key = luaL_checkstring(L, 2);
    if (!key) {
    	return luaL_error(L, "key missing");
    }

    // Open
    err = nvs_open(nspace, NVS_READONLY, &handle_to_settings);
    if (err != ESP_OK) {
    
        if (err == ESP_ERR_NVS_NOT_FOUND && total == 3) {
            lua_pushvalue(L, 3);
            return 1;
        }

    	nvs_error(L, err);
    }

    // Read key size
    size_t key_size = 0;
    err = nvs_get_blob(handle_to_settings, key,NULL, &key_size);
    if (err != ESP_OK) {

			if (err == ESP_ERR_NVS_NOT_FOUND && total == 3) {
				nvs_close(handle_to_settings);

				lua_pushvalue(L, 3);
		    return 1;
			}

    	nvs_close(handle_to_settings);
    	nvs_error(L, err);
    	return 0;
    }

    // Alloc space for retrieve key value
    val_val = malloc(key_size);
    if (!val_val) {
    	nvs_close(handle_to_settings);
    	nvs_error(L, ESP_ERR_NO_MEM);
    	return 0;
    }

    // Read key value
    err = nvs_get_blob(handle_to_settings, key,val_val, &key_size);
    if (err != ESP_OK) {
    	nvs_close(handle_to_settings);
    	nvs_error(L, err);
    	return 0;
    }

    val_type = *((char*)val_val);
    switch (val_type) {
    	case LNVS_TYPE_INT:
    		lua_pushinteger(L, *((lua_Integer *)(val_val + 1)));
    		break;
    	case LNVS_TYPE_NUMBER:
    		lua_pushnumber(L, *((lua_Number *)(val_val + 1)));
    		break;
    	case LNVS_TYPE_BOOLEAN:
    		lua_pushboolean(L, *((int *)(val_val + 1)));
    		break;
    	case LNVS_TYPE_NIL:
    		lua_pushnil(L);
    		break;
    	case LNVS_TYPE_STRING:
    		lua_pushstring(L, ((const char *)(val_val + 1)));
    		break;
    }

    free(val_val);

    // Close
    nvs_close(handle_to_settings);

    return 1;
}

// Lua: nvs.exists(namespace, key) return: true|false
static int l_nvs_exists(lua_State *L) {
    int total = lua_gettop(L); // Get number of arguments
    nvs_handle handle_to_settings;
    esp_err_t err;
    const char *key = NULL;
    const char *nspace = NULL;

    // Sanity checks, and check arguments
    if (total < 2 ) {
    	return luaL_error(L, "missing arguments");
    }

    nspace = luaL_checkstring(L, 1);
    if (!nspace) {
    	return luaL_error(L, "namespace missing");
    }

    key = luaL_checkstring(L, 2);
    if (!key) {
    	return luaL_error(L, "key missing");
    }

    // Try to open
    err = nvs_open(nspace, NVS_READONLY, &handle_to_settings);
    if (err != ESP_OK) {
	    lua_pushboolean(L, 0);
    	return 1;
    }
    
    // Try to read key size
    size_t key_size = 0;
    err = nvs_get_blob(handle_to_settings, key, NULL, &key_size);
    if (err != ESP_OK) {
    	nvs_close(handle_to_settings);
			lua_pushboolean(L, 0);
	    return 1;
    }

    // Close
    nvs_close(handle_to_settings);

    lua_pushboolean(L, 1);
    return 1;
}

// Lua: nvs.rm(namespace, key) return: true|false
static int l_nvs_rm(lua_State *L) {
    int total = lua_gettop(L); // Get number of arguments
    nvs_handle handle_to_settings;
    esp_err_t err;
    const char *key = NULL;
    const char *nspace = NULL;

    // Sanity checks, and check arguments
    if (total < 2 ) {
    	return luaL_error(L, "missing arguments");
    }

    nspace = luaL_checkstring(L, 1);
    if (!nspace) {
    	return luaL_error(L, "namespace missing");
    }

    key = luaL_checkstring(L, 2);
    if (!key) {
    	return luaL_error(L, "key missing");
    }

    // Try to open
    err = nvs_open(nspace, NVS_READWRITE, &handle_to_settings);
    if (err != ESP_OK) {
	    lua_pushboolean(L, 0);
    	return 1;
    }
    
    // Try to read key size
    err = nvs_erase_key(handle_to_settings, key);
    if (err != ESP_OK) {
    	nvs_close(handle_to_settings);
			lua_pushboolean(L, 0);
	    return 1;
    }

    // Commit changes
    err = nvs_commit(handle_to_settings);
    if (err != ESP_OK) {
    	nvs_error(L, err);
    }

    // Close
    nvs_close(handle_to_settings);

    lua_pushboolean(L, 1);
    return 1;
}

static const LUA_REG_TYPE nvs_map[] =
{
  { LSTRKEY( "write" ),      LFUNCVAL( l_nvs_write ) },
  { LSTRKEY( "read" ),       LFUNCVAL( l_nvs_read ) },
  { LSTRKEY( "exists" ),     LFUNCVAL( l_nvs_exists ) },
  { LSTRKEY( "rm" ),     	 LFUNCVAL( l_nvs_rm ) },
  { LNILKEY, LNILVAL }
};

int luaopen_nvs(lua_State *L) {
	#if !LUA_USE_ROTABLE
	luaL_newlib(L, nvs_map);
	return 1;
	#else
	return 0;
	#endif		   
}
	   
MODULE_REGISTER_ROM(NVS, nvs, nvs_map, luaopen_nvs, 1);

#endif

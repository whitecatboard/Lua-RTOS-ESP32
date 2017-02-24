/*
 * Lua RTOS, nvs (non volatile storage) Lua Module
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#if LUA_USE_NVS

#include "xtensa/xos_types.h"
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

static int l_nvs_read(lua_State *L) {
    int total = lua_gettop(L); // Get number of arguments
    nvs_handle handle_to_settings;
    esp_err_t err;
    const char *key = NULL;
    const char *nspace = NULL;
    int8_t val_type = 0;
    void *val_val = NULL;

    // Sanity checks, and check arguments
    if (total != 2 ) {
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
    err = nvs_open(nspace, NVS_READWRITE, &handle_to_settings);
    if (err != ESP_OK) {
    	nvs_error(L, err);
    }

    // Read key size
    size_t key_size = 0;
    err = nvs_get_blob(handle_to_settings, key,NULL, &key_size);
	if (err != ESP_OK) {
		nvs_error(L, err);
	}

	// Alloc space for retrieve key value
	val_val = malloc(key_size);
	if (!val_val) {
		// TO DO
	}

    // Read key value
    err = nvs_get_blob(handle_to_settings, key,val_val, &key_size);
	if (err != ESP_OK) {
		nvs_error(L, err);
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

static const LUA_REG_TYPE nvs_map[] =
{
  { LSTRKEY( "write" ),      LFUNCVAL( l_nvs_write ) },
  { LSTRKEY( "read" ),       LFUNCVAL( l_nvs_read ) },
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
	   
MODULE_REGISTER_MAPPED(NVS, nvs, nvs_map, luaopen_nvs);

#endif

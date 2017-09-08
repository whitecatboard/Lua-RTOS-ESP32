/*
 * Lua RTOS, Lua ULP module
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#include "sdkconfig.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"
#include "modules.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <drivers/cpu.h>
#include "rom/rtc.h"
#include "esp_deep_sleep.h"
#include "esp32/ulp.h"

extern LUA_REG_TYPE ulp_error_map[];

// Module errors
#define LUA_ULP_ERR_CANT_LOAD_BINARY  (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  0)
#define LUA_ULP_ERR_CANT_RUN_BINARY   (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  1)

DRIVER_REGISTER_ERROR(ULP, ulp, CannotLoadBinary,  "can't load ulp binary",  LUA_ULP_ERR_CANT_LOAD_BINARY);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotRunBinary,   "can't run ulp binary",   LUA_ULP_ERR_CANT_RUN_BINARY);

static int lulp_load(lua_State *L) {
	const char* binary = luaL_checkstring(L, 1);
	uint32_t load_addr = 0;
	if (lua_gettop(L) > 1) {
		load_addr = luaL_checkinteger(L, 2);
	}
	
	FILE *fp;
	fp = fopen(binary, "rb");
	if (!fp) {
		fclose(fp);
		return luaL_fileresult(L, 0, binary);
	}

	struct stat fileinfo;
	stat(binary, &fileinfo);

	if ((size_t)fileinfo.st_size > CONFIG_ULP_COPROC_RESERVE_MEM) {
		return luaL_error(L, "cannot load binary file, maximum allowed size is " XTSTR(CONFIG_ULP_COPROC_RESERVE_MEM));
	}

	size_t program_size = (size_t)fileinfo.st_size;
	uint8_t program_binary[program_size];
	size_t nread = fread(program_binary, 1, sizeof(program_binary), fp);
	fclose(fp);

	if (nread != (size_t)fileinfo.st_size) {
		return luaL_error(L, "error reading file");
	}

	esp_err_t error;
	if ((error = ulp_load_binary(load_addr, program_binary, program_size / sizeof(uint32_t)))) {
		return luaL_exception(L, LUA_ULP_ERR_CANT_LOAD_BINARY);
	}

	return 0;
}

static int lulp_run(lua_State *L) {
	uint32_t entry_point = 0;
	if (lua_gettop(L) > 0) {
		entry_point = luaL_checkinteger(L, 1);
	}

	esp_err_t error;
	if ((error = ulp_run(entry_point))) {
		return luaL_exception(L, LUA_ULP_ERR_CANT_RUN_BINARY);
	}

	status_set(STATUS_NEED_RTC_SLOW_MEM); //XXX should depend on assignment of variables

	return 0;
}

static const LUA_REG_TYPE lulp_map[] = {
  { LSTRKEY( "load" ),                   LFUNCVAL( lulp_load ) },
  { LSTRKEY( "run" ),                    LFUNCVAL( lulp_run  ) },

	{LSTRKEY("error"), 			               LROVAL( ulp_error_map    )},
	
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_ulp( lua_State *L ) {
#if !LUA_USE_ROTABLE
  luaL_newlib(L, ulp);
  return 1;
#else
	return 0;
#endif
}

MODULE_REGISTER_MAPPED(ULP, ulp, lulp_map, luaopen_ulp);
DRIVER_REGISTER(ULP,ulp,NULL,NULL,NULL);


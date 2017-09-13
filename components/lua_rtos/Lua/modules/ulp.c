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

// Module errors
#define LUA_ULP_ERR_CANT_LOAD_BINARY       (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  0)
#define LUA_ULP_ERR_CANT_LOAD_BINARY_SIZE  (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  1)
#define LUA_ULP_ERR_CANT_LOAD_BINARY_ARG   (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  2)
#define LUA_ULP_ERR_CANT_LOAD_BINARY_MAGIC (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  3)
#define LUA_ULP_ERR_CANT_RUN_BINARY        (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  4)

DRIVER_REGISTER_ERROR(ULP, ulp, CannotLoadBinary,       "can't load ulp binary",  LUA_ULP_ERR_CANT_LOAD_BINARY);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotLoadBinarySize,   "can't load ulp binary: invalid size",  LUA_ULP_ERR_CANT_LOAD_BINARY_SIZE);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotLoadBinaryArg,    "can't load ulp binary: invalid load addr",  LUA_ULP_ERR_CANT_LOAD_BINARY_ARG);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotLoadBinaryMagic,  "can't load ulp binary: invalid binary",  LUA_ULP_ERR_CANT_LOAD_BINARY_MAGIC);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotRunBinary,        "can't run ulp binary",   LUA_ULP_ERR_CANT_RUN_BINARY);

typedef struct {
    lua_State *L;
    unsigned long addr;
} ulp_userdata;

static int lulp_load(lua_State *L) {
	const char* binary = luaL_checkstring(L, 1);
	uint32_t load_addr = luaL_optinteger(L, 2, 0);

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
		if (ESP_ERR_INVALID_SIZE  == error) return luaL_exception(L, LUA_ULP_ERR_CANT_LOAD_BINARY_SIZE);
		if (ESP_ERR_INVALID_ARG   == error) return luaL_exception(L, LUA_ULP_ERR_CANT_LOAD_BINARY_ARG);
		if (ESP_ERR_NOT_SUPPORTED == error) return luaL_exception(L, LUA_ULP_ERR_CANT_LOAD_BINARY_MAGIC);
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

	//XXX status_set(STATUS_NEED_RTC_SLOW_MEM); //XXX should depend on assignment of variables
	//XXX esp_err_t ulp_set_wakeup_period(size_t period_index, uint32_t period_us);

	/* XXX
  adc1_ulp_enable();

  //Set low and high thresholds, approx. 1.35V - 1.75V
  ulp_low_thr = 1500;
  ulp_high_thr = 2000;
  */

	return 0;
}

static int lulp_valueat( lua_State* L ) {
	unsigned long addr = luaL_checkinteger(L, 1);

	if (lua_gettop(L) > 1) {
	  uint32_t ulp_value = luaL_checkinteger(L, 2);
	  uint32_t *pValue = (uint32_t *)addr;
  	*pValue = (ulp_value & UINT16_MAX);
	  return 0;
	}
	else
	{
	  uint32_t *pValue = (uint32_t *)addr;
		lua_pushinteger(L, (*pValue) & UINT16_MAX);
		return 1;
	}
}

static int lulp_assign( lua_State* L ){
	unsigned long addr = luaL_checkinteger(L, 1);

	ulp_userdata *ulpvar = (ulp_userdata *)lua_newuserdata(L, sizeof(ulp_userdata));
	ulpvar->L = L;
	ulpvar->addr = addr;

	luaL_getmetatable(L, "ulp.var");
	lua_setmetatable(L, -2);

	return 1;
}

static int lulp_address( lua_State* L ) {
	ulp_userdata *ulpvar = (ulp_userdata *)luaL_checkudata(L, 1, "ulp.var");
	luaL_argcheck(L, ulpvar, 1, "ulp var expected");

	if (lua_gettop(L) > 0) {
	  unsigned long addr = luaL_checkinteger(L, 1);
  	ulpvar->addr = addr;
	  return 0;
	}
	else
	{
		lua_pushinteger(L, ulpvar->addr);
		return 1;
	}
}

static int lulp_value( lua_State* L ) {
	ulp_userdata *ulpvar = (ulp_userdata *)luaL_checkudata(L, 1, "ulp.var");
	luaL_argcheck(L, ulpvar, 1, "ulp var expected");

	if (lua_gettop(L) > 0) {
	  uint32_t ulp_value = luaL_checkinteger(L, 1);
	  uint32_t *pValue = (uint32_t *)ulpvar->addr;
  	*pValue = (ulp_value & UINT16_MAX);
	  return 0;
	}
	else
	{
	  uint32_t *pValue = (uint32_t *)ulpvar->addr;
		lua_pushinteger(L, (*pValue) & UINT16_MAX);
		return 1;
	}
}

static const LUA_REG_TYPE lulp_map[] = {
  { LSTRKEY( "load" ),                   LFUNCVAL( lulp_load    ) },
  { LSTRKEY( "run" ),                    LFUNCVAL( lulp_run     ) },
  { LSTRKEY( "valueat" ),                LFUNCVAL( lulp_valueat ) },
  { LSTRKEY( "assign" ),                 LFUNCVAL( lulp_assign  ) },

	{ LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lulp_service_map[] = {
  { LSTRKEY( "address"     ),   LFUNCVAL( lulp_address          ) },
  { LSTRKEY( "value"       ),   LFUNCVAL( lulp_value            ) },
  { LSTRKEY( "__metatable" ),   LROVAL  ( lulp_service_map      ) },
  { LSTRKEY( "__index"     ),   LROVAL  ( lulp_service_map      ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_ulp( lua_State *L ) {
  luaL_newmetarotable(L,"ulp.var", (void *)lulp_service_map);

#if !LUA_USE_ROTABLE
  luaL_newlib(L, ulp);
  return 1;
#else
	return 0;
#endif
}

MODULE_REGISTER_MAPPED(ULP, ulp, lulp_map, luaopen_ulp);
DRIVER_REGISTER(ULP,ulp,NULL,NULL,NULL);


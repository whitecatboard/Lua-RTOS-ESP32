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
#include <sys/status.h>

#include <drivers/cpu.h>
#include <rom/rtc.h>
#include <esp_deep_sleep.h>
#include <esp32/ulp.h>
#include <soc/rtc_cntl_reg.h>

// Module errors
#define LUA_ULP_ERR_CANT_LOAD_BINARY       (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  0)
#define LUA_ULP_ERR_CANT_LOAD_BINARY_SIZE  (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  1)
#define LUA_ULP_ERR_CANT_LOAD_BINARY_ARG   (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  2)
#define LUA_ULP_ERR_CANT_LOAD_BINARY_MAGIC (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  3)
#define LUA_ULP_ERR_CANT_RUN_BINARY        (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  4)
#define LUA_ULP_ERR_CANT_SET_TIMER_1       (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  5)
#define LUA_ULP_ERR_CANT_SET_TIMER_2       (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  6)
#define LUA_ULP_ERR_CANT_SET_TIMER_3       (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  7)
#define LUA_ULP_ERR_CANT_SET_TIMER_4       (DRIVER_EXCEPTION_BASE(ULP_DRIVER_ID) |  8)

DRIVER_REGISTER_ERROR(ULP, ulp, CannotLoadBinary,       "can't load ulp binary",  LUA_ULP_ERR_CANT_LOAD_BINARY);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotLoadBinarySize,   "can't load ulp binary: invalid size",  LUA_ULP_ERR_CANT_LOAD_BINARY_SIZE);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotLoadBinaryArg,    "can't load ulp binary: invalid load addr",  LUA_ULP_ERR_CANT_LOAD_BINARY_ARG);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotLoadBinaryMagic,  "can't load ulp binary: invalid binary",  LUA_ULP_ERR_CANT_LOAD_BINARY_MAGIC);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotRunBinary,        "can't run ulp binary",   LUA_ULP_ERR_CANT_RUN_BINARY);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotSetTimer1,        "can't set timer 1",      LUA_ULP_ERR_CANT_SET_TIMER_1);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotSetTimer2,        "can't set timer 2",      LUA_ULP_ERR_CANT_SET_TIMER_2);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotSetTimer3,        "can't set timer 3",      LUA_ULP_ERR_CANT_SET_TIMER_3);
DRIVER_REGISTER_ERROR(ULP, ulp, CannotSetTimer4,        "can't set timer 4",      LUA_ULP_ERR_CANT_SET_TIMER_4);

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

static int lulp_timer(lua_State *L) {
	uint32_t tmr1us = luaL_optinteger(L, 1, 0);
	uint32_t tmr2us = luaL_optinteger(L, 2, 0);
	uint32_t tmr3us = luaL_optinteger(L, 3, 0);
	uint32_t tmr4us = luaL_optinteger(L, 4, 0);
	
	esp_err_t error;
	if ((error = ulp_set_wakeup_period((size_t)0, tmr1us)))
		return luaL_exception(L, LUA_ULP_ERR_CANT_SET_TIMER_1);
	if ((error = ulp_set_wakeup_period((size_t)1, tmr2us)))
		return luaL_exception(L, LUA_ULP_ERR_CANT_SET_TIMER_2);
	if ((error = ulp_set_wakeup_period((size_t)2, tmr3us)))
		return luaL_exception(L, LUA_ULP_ERR_CANT_SET_TIMER_3);
	if ((error = ulp_set_wakeup_period((size_t)3, tmr4us)))
		return luaL_exception(L, LUA_ULP_ERR_CANT_SET_TIMER_4);

	return 0;
}

static int lulp_run(lua_State *L) {
	uint32_t entry_point = 0;
	if (lua_gettop(L) > 0) {
		entry_point = luaL_checkinteger(L, 1);
	}

	status_set(STATUS_NEED_RTC_SLOW_MEM); //used for ulp variables

	esp_err_t error;
	if ((error = ulp_run(entry_point))) {
		return luaL_exception(L, LUA_ULP_ERR_CANT_RUN_BINARY);
	}

	return 0;
}

static int lulp_stop(lua_State *L) {
  // disable ULP timer
  CLEAR_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN);
  // wait for at least 1 RTC_SLOW_CLK cycle
	ets_delay_us(10);

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

	if (lua_gettop(L) > 1) {
	  unsigned long addr = luaL_checkinteger(L, 2);
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

	if (lua_gettop(L) > 1) {
	  uint32_t ulp_value = luaL_checkinteger(L, 2);
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

static int lulp_setadc( lua_State* L ) {
	/* XXX
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_11db);
  adc1_config_width(ADC_WIDTH_12Bit);
  adc1_ulp_enable();
  */
  printf("initializing the ADC for use with the ULP is not yet supported.\n");

  return 0;
}

static const LUA_REG_TYPE lulp_map[] = {
  { LSTRKEY( "load" ),                   LFUNCVAL( lulp_load    ) },
  { LSTRKEY( "settimer" ),               LFUNCVAL( lulp_timer   ) },
  { LSTRKEY( "run" ),                    LFUNCVAL( lulp_run     ) },
  { LSTRKEY( "stop" ),                   LFUNCVAL( lulp_stop    ) },
  { LSTRKEY( "enableadc" ),              LFUNCVAL( lulp_setadc  ) },
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


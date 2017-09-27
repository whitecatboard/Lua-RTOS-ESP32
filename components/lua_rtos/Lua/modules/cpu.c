/*
 * Lua RTOS, Lua CPU module
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

#include <drivers/cpu.h>
#include "rom/rtc.h"
#include "esp_sleep.h"

extern const int cpu_error_map;

// Module errors
#define LUA_CPU_ERR_CANT_WAKEON_EXT0   (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  0)
#define LUA_CPU_ERR_CANT_WAKEON_EXT1   (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  1)
#define LUA_CPU_ERR_CANT_WAKEON_TIMER  (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  2)
#define LUA_CPU_ERR_CANT_WAKEON_TOUCH  (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  3)
#define LUA_CPU_ERR_CANT_WAKEON_ULP    (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  4)

// Register drivers and errors
DRIVER_REGISTER_BEGIN(CPU,cpu,NULL,NULL,NULL);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotWakeOnExt0,  "can't wake on EXT0",  LUA_CPU_ERR_CANT_WAKEON_EXT0);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotWakeOnExt1,  "can't wake on EXT1",  LUA_CPU_ERR_CANT_WAKEON_EXT1);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotWakeOnTimer, "can't wake on timer", LUA_CPU_ERR_CANT_WAKEON_TIMER);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotWakeOnTouch, "can't wake on touch", LUA_CPU_ERR_CANT_WAKEON_TOUCH);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotWakeOnULP,   "can't wake on ULP",   LUA_CPU_ERR_CANT_WAKEON_ULP);
DRIVER_REGISTER_END(CPU,cpu,NULL,NULL,NULL);

static int lcpu_model(lua_State *L) {
	int revision;

	char model[18];
	char cpuInfo[26];

	cpu_model(model);
	revision = cpu_revision();
	if (revision) {
		sprintf(cpuInfo, "%s rev A%d", model, cpu_revision());
	} else {
		sprintf(cpuInfo, "%s", model);
	}

	lua_pushstring(L, cpuInfo);

	return 1;
}

static int lcpu_board(lua_State *L) {
	lua_pushstring(L, LUA_RTOS_BOARD);
	return 1;
}

static int lcpu_sleep(lua_State *L) {
	unsigned int seconds = luaL_checkinteger(L, 1);
	cpu_sleep(seconds);
	return 0;
}

static int lcpu_reset_reason(lua_State *L) {
	lua_pushinteger(L, cpu_reset_reason());
	return 1;
}

static int lcpu_wakeup_reason(lua_State *L) {
	lua_pushinteger(L, cpu_wakeup_reason());
	return 1;
}

static int lcpu_wakeup_on(lua_State *L) {
	esp_err_t error;
	unsigned int type = luaL_checkinteger(L, 1);

	switch(type) {
		case ESP_SLEEP_WAKEUP_EXT0:
				{
					unsigned int gpio = luaL_checkinteger(L, 2);
					unsigned int level = luaL_checkinteger(L, 3);
					if ((error = esp_sleep_enable_ext0_wakeup(gpio, level))) {
						return luaL_exception(L, LUA_CPU_ERR_CANT_WAKEON_EXT0);
					}
				}
				break;
		case ESP_SLEEP_WAKEUP_EXT1:
				{
					uint64_t mask = luaL_checkinteger(L, 2);
					unsigned int wakeup_mode = luaL_checkinteger(L, 3);
					/*
							ESP_EXT1_WAKEUP_ALL_LOW = 0,    //!< Wake the chip when all selected GPIOs go low
							ESP_EXT1_WAKEUP_ANY_HIGH = 1    //!< Wake the chip when any of the selected GPIOs go high
					*/
					if ((error = esp_sleep_enable_ext1_wakeup(mask, wakeup_mode))) {
						return luaL_exception(L, LUA_CPU_ERR_CANT_WAKEON_EXT1);
					}
				}
				break;
		case ESP_SLEEP_WAKEUP_TIMER:
				{
					unsigned long time_in_us = luaL_checkinteger(L, 2);
					if ((error = esp_sleep_enable_timer_wakeup(time_in_us))) {
						return luaL_exception(L, LUA_CPU_ERR_CANT_WAKEON_TIMER);
					}
				}
				break;
		case ESP_SLEEP_WAKEUP_TOUCHPAD:
				if ((error = esp_sleep_enable_touchpad_wakeup())) {
					return luaL_exception(L, LUA_CPU_ERR_CANT_WAKEON_TOUCH);
				}
				break;
		case ESP_SLEEP_WAKEUP_ULP:
				if ((error = esp_sleep_enable_ulp_wakeup())) {
					return luaL_exception(L, LUA_CPU_ERR_CANT_WAKEON_ULP);
				}
				break;
	}

	return 0;
}

static int lcpu_deepsleep(lua_State *L) {
	cpu_deepsleep();
	return 0;
}

static int lcpu_wakeup_ext1_mask(lua_State *L) {
	uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
	lua_pushinteger(L, wakeup_pin_mask);
	return 1;
}

static int lcpu_wakeup_ext1_pin(lua_State *L) {
	if (ESP_SLEEP_WAKEUP_EXT1 == cpu_wakeup_reason()) {
		uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();

		if (wakeup_pin_mask != 0) {
				int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
				//printf("Woke up from GPIO %d\n", pin);
				lua_pushinteger(L, pin);
				return 1;
		} else {
				//printf("Woke up from unknown GPIO\n");
				return 0;
		}
		
	}
	return 0;
}

static const LUA_REG_TYPE lcpu_map[] = {
  { LSTRKEY( "model" ),                  LFUNCVAL( lcpu_model ) },
  { LSTRKEY( "board" ),                  LFUNCVAL( lcpu_board ) },
  { LSTRKEY( "sleep" ),                  LFUNCVAL( lcpu_sleep ) },
  { LSTRKEY( "resetreason" ),            LFUNCVAL( lcpu_reset_reason ) },
  { LSTRKEY( "wakeupreason" ),           LFUNCVAL( lcpu_wakeup_reason ) },
  { LSTRKEY( "wakeupon" ),               LFUNCVAL( lcpu_wakeup_on ) },
  { LSTRKEY( "deepsleep" ),              LFUNCVAL( lcpu_deepsleep ) },
  { LSTRKEY( "wakeupext1pin" ),          LFUNCVAL( lcpu_wakeup_ext1_pin ) },
  { LSTRKEY( "wakeupext1mask" ),         LFUNCVAL( lcpu_wakeup_ext1_mask ) },

  { LSTRKEY( "RESET_POWERON" ),          LINTVAL( POWERON_RESET          ) },
  { LSTRKEY( "RESET_SW" ),               LINTVAL( SW_RESET               ) },
  { LSTRKEY( "RESET_DEEPSLEEP" ),        LINTVAL( DEEPSLEEP_RESET        ) },
  { LSTRKEY( "RESET_SDIO" ),             LINTVAL( SDIO_RESET             ) },
  { LSTRKEY( "RESET_TG0WDT_SYS" ),       LINTVAL( TG0WDT_SYS_RESET       ) },
  { LSTRKEY( "RESET_TG1WDT_SYS" ),       LINTVAL( TG1WDT_SYS_RESET       ) },
  { LSTRKEY( "RESET_RTCWDT_SYS" ),       LINTVAL( RTCWDT_SYS_RESET       ) },
  { LSTRKEY( "RESET_INTRUSION" ),        LINTVAL( INTRUSION_RESET        ) },
  { LSTRKEY( "RESET_TGWDT_CPU" ),        LINTVAL( TGWDT_CPU_RESET        ) },
  { LSTRKEY( "RESET_SW_CPU" ),           LINTVAL( SW_CPU_RESET           ) },
  { LSTRKEY( "RESET_RTCWDT_CPU" ),       LINTVAL( RTCWDT_CPU_RESET       ) },
  { LSTRKEY( "RESET_EXT_CPU" ),          LINTVAL( EXT_CPU_RESET          ) },
  { LSTRKEY( "RESET_RTCWDT_BROWN_OUT" ), LINTVAL( RTCWDT_BROWN_OUT_RESET ) },
  { LSTRKEY( "RESET_RTCWDT_RTC" ),       LINTVAL( RTCWDT_RTC_RESET       ) },

  { LSTRKEY( "WAKEUP_NONE" ),            LINTVAL( ESP_SLEEP_WAKEUP_UNDEFINED ) },
  { LSTRKEY( "WAKEUP_EXT0" ),            LINTVAL( ESP_SLEEP_WAKEUP_EXT0      ) },
  { LSTRKEY( "WAKEUP_EXT1" ),            LINTVAL( ESP_SLEEP_WAKEUP_EXT1      ) },
  { LSTRKEY( "WAKEUP_TIMER" ),           LINTVAL( ESP_SLEEP_WAKEUP_TIMER     ) },
  { LSTRKEY( "WAKEUP_TOUCHPAD" ),        LINTVAL( ESP_SLEEP_WAKEUP_TOUCHPAD  ) },
  { LSTRKEY( "WAKEUP_ULP" ),             LINTVAL( ESP_SLEEP_WAKEUP_ULP       ) },

	DRIVER_REGISTER_LUA_ERRORS(cpu)
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_cpu( lua_State *L ) {
#if !LUA_USE_ROTABLE
  luaL_newlib(L, cpu);
  return 1;
#else
	return 0;
#endif
}

MODULE_REGISTER_MAPPED(CPU, cpu, lcpu_map, luaopen_cpu);


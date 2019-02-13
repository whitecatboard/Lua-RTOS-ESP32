/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, Lua CPU module
 *
 */

#include "sdkconfig.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "error.h"
#include "modules.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <drivers/cpu.h>
#include <rom/rtc.h>
#include <esp_sleep.h>
#include <esp_panic.h>
#include <soc/rtc.h>
#include <esp32/pm.h>
#include <esp_pm.h>

extern const int cpu_error_map;

// Module errors
#define LUA_CPU_ERR_CANT_WAKEON_EXT0    (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  0)
#define LUA_CPU_ERR_CANT_WAKEON_EXT1    (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  1)
#define LUA_CPU_ERR_CANT_WAKEON_TIMER   (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  2)
#define LUA_CPU_ERR_CANT_WAKEON_TOUCH   (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  3)
#define LUA_CPU_ERR_CANT_WAKEON_ULP     (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  4)
#define LUA_CPU_ERR_CANT_SET_WATCHPOINT (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  5)
#define LUA_CPU_ERR_INVALID_CPU_SPEED   (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  6)
#define LUA_CPU_ERR_CANT_SET_CPU_SPEED  (DRIVER_EXCEPTION_BASE(CPU_DRIVER_ID) |  7)

// Register drivers and errors
DRIVER_REGISTER_BEGIN(CPU,cpu,0,NULL,NULL);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotWakeOnExt0,    "can't wake on EXT0",   LUA_CPU_ERR_CANT_WAKEON_EXT0);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotWakeOnExt1,    "can't wake on EXT1",   LUA_CPU_ERR_CANT_WAKEON_EXT1);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotWakeOnTimer,   "can't wake on timer",  LUA_CPU_ERR_CANT_WAKEON_TIMER);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotWakeOnTouch,   "can't wake on touch",  LUA_CPU_ERR_CANT_WAKEON_TOUCH);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotWakeOnULP,     "can't wake on ULP",    LUA_CPU_ERR_CANT_WAKEON_ULP);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotSetWatchpoint, "can't set Watchpoint", LUA_CPU_ERR_CANT_SET_WATCHPOINT);
	DRIVER_REGISTER_ERROR(CPU, cpu, CPUSpeedInvalid,     "invalid CPU speed",    LUA_CPU_ERR_INVALID_CPU_SPEED);
	DRIVER_REGISTER_ERROR(CPU, cpu, CannotSetCPUSpeed,   "can't set CPU speed",  LUA_CPU_ERR_CANT_SET_CPU_SPEED);
DRIVER_REGISTER_END(CPU,cpu,0,NULL,NULL);

int temprature_sens_read(void); //undocumented esp32 function

static int lcpu_model(lua_State *L) {
	int revision;

	char model[18];
	char cpuInfo[26];

	cpu_model(model, sizeof(model));
	revision = cpu_revision();
	if (revision) {
		snprintf(cpuInfo, sizeof(cpuInfo), "%s rev A%d", model, cpu_revision());
	} else {
		snprintf(cpuInfo, sizeof(cpuInfo), "%s", model);
	}

	lua_pushstring(L, cpuInfo);

	return 1;
}

static int lcpu_board(lua_State *L) {
	lua_pushstring(L, LUA_RTOS_BOARD);
	lua_pushstring(L, CONFIG_LUA_RTOS_BOARD_SUBTYPE);
	lua_pushstring(L, CONFIG_LUA_RTOS_BOARD_BRAND);
	return 3;
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
				// needs the FSM mode of the touch button to be configured in the timer trigger mode.
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

static int lcpu_watchpoint(lua_State *L) {
	uint32_t addr = luaL_checkinteger(L, 1);
	int size = luaL_optinteger(L, 2, 4); //must be one of 2^n, with n in [0..6]
	int flags = luaL_optinteger(L, 3, ESP_WATCHPOINT_STORE); //when to break

	if (size!=1 && size!=2 && size!=4 && size!=8 && size!=16 && size!=32 && size!=64)
		return luaL_exception(L, LUA_CPU_ERR_CANT_SET_WATCHPOINT);

	esp_set_watchpoint(0, (void *)addr, size, flags); //watchpoint 1 may be used by freertos CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK
	return 0;
}

static int lcpu_temperature(lua_State *L) {
	lua_pushnumber(L, ((float)temprature_sens_read() - 64.0) / 1.8 );
	return 1;
}

static int lcpu_speed(lua_State *L) {
#ifdef CONFIG_PM_ENABLE
	if (lua_gettop(L) > 0) {
		esp_err_t error;
		rtc_cpu_freq_t max_freq;

		int speed = luaL_checkinteger(L, 1);
		bool dynamic = false;
		if (lua_gettop(L) > 1) {
			luaL_checktype(L, 2, LUA_TBOOLEAN);
			dynamic = lua_toboolean(L, 2);
		}

		if (speed > 10) {
			//enable use of sdkconfig value via CPU_SPEED_DEFAULT
			//speed is an actual mhz value so we need to convert it
			if (!rtc_clk_cpu_freq_from_mhz(CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ, &max_freq)) {
				return luaL_exception(L, LUA_CPU_ERR_INVALID_CPU_SPEED);
			}
		}
		else {
			max_freq = speed;
		}

		esp_pm_config_esp32_t pm_config = {
				    .max_cpu_freq = max_freq,
				    .min_cpu_freq = (dynamic ? RTC_CPU_FREQ_XTAL : max_freq)
		};

		if ((error = esp_pm_configure(&pm_config))) {
			return luaL_exception(L, LUA_CPU_ERR_CANT_SET_CPU_SPEED);
		}

		//need to usleep here for the return value to be correct
		usleep(1000);
	}
#endif

	lua_pushinteger(L, cpu_speed() / 1000000);
	return 1;
}

#define MAX_BACKTRACE 100
extern __NOINIT_ATTR uint32_t backtrace_count;
extern __NOINIT_ATTR uint32_t backtrace_pc[MAX_BACKTRACE];
extern __NOINIT_ATTR uint32_t backtrace_sp[MAX_BACKTRACE];

static int lcpu_backtrace(lua_State *L) {
	printf("\r\nBacktrace:");

	int reason = cpu_reset_reason();
	if (POWERON_RESET == reason || RTCWDT_RTC_RESET == reason || EXT_CPU_RESET == reason) {
		printf(" none\r\n");
		return 0;
	}

	lua_newtable(L);
	for (uint32_t idx = 0; idx < MAX_BACKTRACE && idx < backtrace_count; idx++) {
		printf(" 0x%08x:0x%08x", backtrace_pc[idx], backtrace_sp[idx]);

		lua_pushnumber(L, idx); //row index
		lua_newtable(L);

		lua_pushinteger(L, backtrace_pc[idx]);
		lua_setfield (L, -2, "pc");

		lua_pushinteger(L, backtrace_sp[idx]);
		lua_setfield (L, -2, "sp");

		lua_settable( L, -3 );
	}
	printf("\r\n");

	return 1; //one table
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
  { LSTRKEY( "watchpoint" ),             LFUNCVAL( lcpu_watchpoint ) },
  { LSTRKEY( "temperature" ),            LFUNCVAL( lcpu_temperature ) },
  { LSTRKEY( "speed" ),                  LFUNCVAL( lcpu_speed ) },
  { LSTRKEY( "backtrace" ),              LFUNCVAL( lcpu_backtrace ) },

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

  { LSTRKEY( "WATCHPOINT_LOAD" ),        LINTVAL( ESP_WATCHPOINT_LOAD    ) },
  { LSTRKEY( "WATCHPOINT_STORE" ),       LINTVAL( ESP_WATCHPOINT_STORE   ) },
  { LSTRKEY( "WATCHPOINT_ACCESS" ),      LINTVAL( ESP_WATCHPOINT_ACCESS  ) },

  { LSTRKEY( "SPEED_DEFAULT" ),          LINTVAL( CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ ) }, //e.g. 240
  { LSTRKEY( "SPEED_FAST" ),             LINTVAL( RTC_CPU_FREQ_240M      ) }, //3
  { LSTRKEY( "SPEED_MEDIUM" ),           LINTVAL( RTC_CPU_FREQ_160M      ) }, //2
  { LSTRKEY( "SPEED_SLOW" ),             LINTVAL( RTC_CPU_FREQ_80M       ) }, //1

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

MODULE_REGISTER_ROM(CPU, cpu, lcpu_map, luaopen_cpu, 1);


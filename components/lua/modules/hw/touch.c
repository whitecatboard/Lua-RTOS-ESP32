/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Thomas E. Horner (whitecatboard.org@horner.it)
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
 * Lua RTOS, Lua TouchPad module
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
#include <sys/stat.h>
#include <sys/status.h>
#include <sys/delay.h>

#include <driver/touch_pad.h>

#define TOUCH_THRESH_NO_USE   (0)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

static int filter_mode = 1;

// Module errors
#define LUA_TOUCH_ERR_CANT_SET_FSM_MODE               (DRIVER_EXCEPTION_BASE(TOUCH_DRIVER_ID) |  0)
#define LUA_TOUCH_ERR_CANT_SET_GROUP_MASK             (DRIVER_EXCEPTION_BASE(TOUCH_DRIVER_ID) |  1)
#define LUA_TOUCH_ERR_CANT_SET_TRIGGER_SOURCE         (DRIVER_EXCEPTION_BASE(TOUCH_DRIVER_ID) |  2)
#define LUA_TOUCH_ERR_CANT_SET_THRESHOLD              (DRIVER_EXCEPTION_BASE(TOUCH_DRIVER_ID) |  3)

// Register drivers and errors
DRIVER_REGISTER_BEGIN(TOUCH,touch,0,NULL,NULL);
	DRIVER_REGISTER_ERROR(TOUCH, touch, CannotSetFsmMode,       "can't set FSM mode",  LUA_TOUCH_ERR_CANT_SET_FSM_MODE);
	DRIVER_REGISTER_ERROR(TOUCH, touch, CannotSetGroupMask,     "can't set group mask",  LUA_TOUCH_ERR_CANT_SET_GROUP_MASK);
	DRIVER_REGISTER_ERROR(TOUCH, touch, CannotSetTriggerSource, "can't set trigger source",  LUA_TOUCH_ERR_CANT_SET_TRIGGER_SOURCE);
	DRIVER_REGISTER_ERROR(TOUCH, touch, CannotSetThreshold,     "can't set threshold",  LUA_TOUCH_ERR_CANT_SET_THRESHOLD);
DRIVER_REGISTER_END(TOUCH,touch,0,NULL,NULL);


static int ltouch_init(lua_State *L) {
	touch_high_volt_t refh = luaL_optinteger(L, 1, TOUCH_HVOLT_2V7);
	touch_low_volt_t refl = luaL_optinteger(L, 2, TOUCH_LVOLT_0V5);
	touch_volt_atten_t atten = luaL_optinteger(L, 3, TOUCH_HVOLT_ATTEN_1V);

	uint32_t filter_period_ms = TOUCHPAD_FILTER_TOUCH_PERIOD;
	if (lua_gettop(L) > 3) {
		luaL_checktype(L, 4, LUA_TBOOLEAN);
		filter_mode = lua_toboolean(L, 4);
		filter_period_ms = luaL_optinteger(L, 5, TOUCHPAD_FILTER_TOUCH_PERIOD);
	}

	touch_pad_init();
	touch_pad_set_voltage(refh, refl, atten);

	//FIXME currently it's only all-or-nothing
	for (int i = 0; i<TOUCH_PAD_MAX; i++) {
		touch_pad_config(i, TOUCH_THRESH_NO_USE);
	}


	if (filter_mode) {
		touch_pad_filter_start(filter_period_ms);
	}


	// wakeup on touch needs the FSM mode of the touch button to be configured in the timer trigger mode
	esp_err_t error;
	if ((error = touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER)))
		return luaL_exception(L, LUA_TOUCH_ERR_CANT_SET_FSM_MODE);
	if ((error = touch_pad_set_group_mask(TOUCH_PAD_BIT_MASK_MAX, TOUCH_PAD_BIT_MASK_MAX, TOUCH_PAD_BIT_MASK_MAX)))
		return luaL_exception(L, LUA_TOUCH_ERR_CANT_SET_GROUP_MASK);
	if ((error = touch_pad_set_trigger_source(TOUCH_TRIGGER_SOURCE_SET1)))
		return luaL_exception(L, LUA_TOUCH_ERR_CANT_SET_TRIGGER_SOURCE);

	return 0;
}

static int ltouch_read(lua_State *L)
{
	uint16_t touch_value;
	uint16_t touch_filter_value;

	//FIXME currently it's only all-or-nothing
	for (int i = 0; i < TOUCH_PAD_MAX; i++) {
		if (filter_mode) {
			// In filter mode, we need to use this API to get the touch pad value.
			touch_pad_read_raw_data(i, &touch_value);
			touch_pad_read_filtered(i, &touch_filter_value);
			lua_pushinteger(L, touch_filter_value);
		} else {
			touch_pad_read(i, &touch_value);
			lua_pushinteger(L, touch_value);
		}
	}

	return TOUCH_PAD_MAX;
}

static int ltouch_wakethreshold(lua_State *L)
{
	int params = lua_gettop(L);
	double threshold;
	esp_err_t error;

	for(int i = 0; i < params; i++) {
		threshold = lua_tonumber(L, i+1);

		if (i<TOUCH_PAD_MAX) {
			//set wakeup threshold.
			if ((error = touch_pad_set_thresh(i, (uint16_t)threshold))) {
				return luaL_exception(L, LUA_TOUCH_ERR_CANT_SET_THRESHOLD);
			}
		}
	}

	status_set(STATUS_NEED_RTC_SLOW_MEM, 0x00000000); //to enable wake up from deep sleep by touch make sure rtc touch is powered
	return 0;
}

static const LUA_REG_TYPE ltouch_map[] = {
  { LSTRKEY( "init" ),                   LFUNCVAL( ltouch_init    ) },
  { LSTRKEY( "read" ),                   LFUNCVAL( ltouch_read    ) },
  { LSTRKEY( "wakethreshold" ),          LFUNCVAL( ltouch_wakethreshold    ) },

  { LSTRKEY("HVOLT_KEEP"), LINTVAL(TOUCH_HVOLT_KEEP) },
  { LSTRKEY("HVOLT_2V4"),  LINTVAL(TOUCH_HVOLT_2V4)  },
  { LSTRKEY("HVOLT_2V5"),  LINTVAL(TOUCH_HVOLT_2V5)  },
  { LSTRKEY("HVOLT_2V6"),  LINTVAL(TOUCH_HVOLT_2V6)  },
  { LSTRKEY("HVOLT_2V7"),  LINTVAL(TOUCH_HVOLT_2V7)  },

  { LSTRKEY("LVOLT_KEEP"), LINTVAL(TOUCH_LVOLT_KEEP) },
  { LSTRKEY("LVOLT_0V5"),  LINTVAL(TOUCH_LVOLT_0V5)  },
  { LSTRKEY("LVOLT_0V6"),  LINTVAL(TOUCH_LVOLT_0V6)  },
  { LSTRKEY("LVOLT_0V7"),  LINTVAL(TOUCH_LVOLT_0V7)  },
  { LSTRKEY("LVOLT_0V8"),  LINTVAL(TOUCH_LVOLT_0V8)  },

  { LSTRKEY("HVOLT_ATTEN_KEEP"), LINTVAL(TOUCH_HVOLT_ATTEN_KEEP) },
  { LSTRKEY("HVOLT_ATTEN_1V5"),  LINTVAL(TOUCH_HVOLT_ATTEN_1V5)  },
  { LSTRKEY("HVOLT_ATTEN_1V"),   LINTVAL(TOUCH_HVOLT_ATTEN_1V)   },
  { LSTRKEY("HVOLT_ATTEN_0V5"),  LINTVAL(TOUCH_HVOLT_ATTEN_0V5)  },
  { LSTRKEY("HVOLT_ATTEN_0V"),   LINTVAL(TOUCH_HVOLT_ATTEN_0V)   },

  DRIVER_REGISTER_LUA_ERRORS(touch)
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_touch( lua_State *L ) {
  LNEWLIB(L, touch);
}

MODULE_REGISTER_ROM(TOUCH, touch, ltouch_map, luaopen_touch, 1);


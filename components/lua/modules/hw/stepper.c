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
 * Lua RTOS, Lua stepper module
 *
 */
#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_STEPPER

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "error.h"
#include "modules.h"

#include <drivers/gpio.h>
#include <drivers/stepper.h>

typedef struct {
    uint8_t unit;
    double stpu;     // Steps per unit
    double min_spd;  // Min speed in units / min
    double max_spd;  // Max speed in units / min
    double accel;    // Acceleration in units/secs^2
} stepper_userdata;

static int lstepper_attach( lua_State* L ){
	driver_error_t *error;
    uint8_t unit = 0;
    
    // Direction & step pins
    int dir_pin  = luaL_checkinteger(L, 1);
    int step_pin = luaL_checkinteger(L, 2);

    double stpu    = luaL_optnumber(L, 3, 200.0); // Steps per unit (200 steps per revolution by default)
    double min_spd = luaL_optnumber(L, 4, 60.0);  // Min speed in units / min (60 rpm by default)
    double max_spd = luaL_optnumber(L, 5, 1000.0); // Max speed in units / min (800 rpm by default)
    double accel   = luaL_optnumber(L, 6, 2);     // Acceleration in units/secs^2 (2)

    if ((error = stepper_setup(step_pin, dir_pin, &unit))) {
    	return luaL_driver_error(L, error);
    }

    // Allocate stepper structure and initialize
    stepper_userdata *stepper = (stepper_userdata *)lua_newuserdata(L, sizeof(stepper_userdata));

    stepper->unit = unit;
    stepper->stpu = stpu;
    stepper->min_spd = min_spd;
    stepper->max_spd = max_spd;
    stepper->accel = accel;

    luaL_getmetatable(L, "stepper.inst");
    lua_setmetatable(L, -2);

    return 1;
}

static int lstepper_move( lua_State* L ){
    stepper_userdata *lstepper = NULL;

    lstepper = (stepper_userdata *)luaL_checkudata(L, 1, "stepper.inst");
    luaL_argcheck(L, lstepper, 1, "stepper expected");

    double units  = luaL_checknumber(L, 2);
    double speed  = luaL_optnumber(L, 3, lstepper->max_spd); // Speed in units / min (800 rpm by default)
    double accel  = luaL_optnumber(L, 4, lstepper->accel);   // Acceleration in units/secs^2 (2)

    double ispd = lstepper->min_spd;
    double stpu = lstepper->stpu;

    // Sanity checks
    if (speed < lstepper->min_spd) {
    	speed = lstepper->min_spd;
    }

    if (speed > lstepper->max_spd) {
    	speed = lstepper->max_spd;
    }

    // Calculate direction
    uint8_t dir = (units >= 0.0);

	// Calculate steps needed to move the axis by the desired units
    double absUnits = fabs(units);
    double steps = absUnits * stpu;

	// Remember:
	//
	// acc = (speed - initial speed) / time
	// distance = initial speed * time + (1/2) * acc * time ^ 2

	// Calculate the time required to reach desired speed from the initial speed at
	// the desired acceleration
    if (speed < ispd) {
    	ispd = speed;
    }

    if (accel <= 0) {
    	accel = 1;
    }

	// Calculate the distance required to reach the desired speed from the initial speed at
	// the desired acceleration
    double acc_time = ((speed - ispd) / (60.0 * accel));
    double acc_dist = (ispd / 60.0) * acc_time + ((accel  * acc_time * acc_time)/2.0);

	// Calculate the steps required to reach the desired speed from the initial speed at
	// the desired acceleration
    double acc_steps = acc_dist * stpu;

	if ((steps - 2.0 * acc_steps) < 0.0) {
		acc_steps = floor(steps / 2.0);
	}

	// Calculate initial and end frequency
	double ifreq = (ispd  * stpu) / 60.0;
	double efreq = (speed * stpu) / 60.0;

    stepper_move(
    		lstepper->unit, dir, (uint32_t)floor(steps), (uint32_t)floor(acc_steps),
			(uint32_t)floor(ifreq), (uint32_t)floor(efreq)
	);

    return 0;
}

static int lstepper_start( lua_State* L ){
    stepper_userdata *lstepper = NULL;
	int mask = 0;
	int i;

	if (lua_istable(L, 1)) {
		lua_pushnil(L);
		while (lua_next(L, 1) != 0) {
			if (!lua_isnil(L, -1)) {
				lstepper = (stepper_userdata *)luaL_checkudata(L, -1, "stepper.inst");
				luaL_argcheck(L, lstepper, -1, "stepper expected");

				mask |= (1 << (lstepper->unit));
			}

			lua_pop(L, 1);
		}
	} else {
	    int total = lua_gettop(L);

	    for (i=1; i <= total; i++) {
	        if (!lua_isnil(L, i)) {
	            lstepper = (stepper_userdata *)luaL_checkudata(L, i, "stepper.inst");
	            luaL_argcheck(L, lstepper, i, "stepper expected");

	            mask |= (1 << (lstepper->unit));
	        }
	    }
	}

	if (mask) {
		stepper_start(mask);
	}

    return 0;
}

static int lstepper_stop( lua_State* L ) {
    stepper_stop(0xffffffff);

    return 0;
}

static const LUA_REG_TYPE lstepper_map[] = {
    { LSTRKEY( "attach" ),		  LFUNCVAL( lstepper_attach    ) },
	{ LSTRKEY( "start"  ),		  LFUNCVAL( lstepper_start     ) },
	{ LSTRKEY( "stop"   ),        LFUNCVAL( lstepper_stop      ) },
	DRIVER_REGISTER_LUA_ERRORS(stepper)
	{ LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lstepper_inst_map[] = {
	{ LSTRKEY( "move"        ),   LFUNCVAL( lstepper_move      ) },
    { LSTRKEY( "__metatable" ),	  LROVAL  ( lstepper_inst_map  ) },
	{ LSTRKEY( "__index"     ),   LROVAL  ( lstepper_inst_map  ) },
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_stepper( lua_State *L ) {
    luaL_newmetarotable(L,"stepper.inst", (void *)lstepper_inst_map);
    return 0;
}

MODULE_REGISTER_ROM(STEPPER, stepper, lstepper_map, luaopen_stepper, 1);

#endif

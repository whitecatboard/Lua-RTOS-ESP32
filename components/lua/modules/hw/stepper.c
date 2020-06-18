/*
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#include <sys/syslog.h>

typedef struct {
    uint8_t unit;
    float stpu;     // Steps per unit
    float min_spd;  // Min speed in units / min
    float max_spd;  // Max speed in units / min
    float max_acc;  // Max acceleration in units/secs^2
    float max_jerk; // Max jerk in units/secs^3
} stepper_userdata;

static int lstepper_attach( lua_State* L ){
    driver_error_t *error;
    uint8_t unit = 0;
    
    // Direction & step pins
    int dir_pin  = luaL_checkinteger(L, 1);
    int step_pin = luaL_checkinteger(L, 2);

    float stpu     = luaL_optnumber(L, 3, 200.0);  // Steps per unit (200 steps per revolution by default)
    float min_spd  = luaL_optnumber(L, 4, 60.0);   // Min speed in units / min (60 rpm by default)
    float max_spd  = luaL_optnumber(L, 5, 1000.0); // Max speed in units / min (1000 rpm by default)
    float max_acc  = luaL_optnumber(L, 6, 2);      // Max acceleration in units/secs^2 (2 by default )
    float max_jerk = luaL_optnumber(L, 7, 20);     // Max acceleration in units/secs^3 (20 by default )

    min_spd = min_spd / 60.0;
    max_spd = max_spd / 60.0;

    if ((error = stepper_setup(step_pin, dir_pin, min_spd, max_spd, max_acc, stpu, &unit))) {
        return luaL_driver_error(L, error);
    }

    // Allocate stepper structure and initialize
    stepper_userdata *stepper = (stepper_userdata *)lua_newuserdata(L, sizeof(stepper_userdata));

    stepper->unit = unit;
    stepper->stpu = stpu;
    stepper->min_spd  = min_spd;
    stepper->max_spd  = max_spd;
    stepper->max_acc  = max_acc;
    stepper->max_jerk = max_jerk;

    luaL_getmetatable(L, "stepper.inst");
    lua_setmetatable(L, -2);

    return 1;
}

static int lstepper_move( lua_State* L ) {
    stepper_userdata *lstepper = NULL;

    lstepper = (stepper_userdata *)luaL_checkudata(L, 1, "stepper.inst");
    luaL_argcheck(L, lstepper, 1, "stepper expected");

    float units  = luaL_checknumber(L, 2);

    // Speed in units / min (1000 rpm by default)
    float speed;

    if lua_isnil(L, 3) {
        speed = lstepper->max_spd;
    } else {
        speed = luaL_checknumber(L, 3);
        speed = speed / 60.0;
    }

    // Acceleration in units/secs^2 (2 by default)
    float accel;

    if lua_isnil(L, 4) {
        accel = lstepper->max_acc;
    } else {
        accel = luaL_checknumber(L, 4);
    }

    float jerk;

    if lua_isnil(L, 5) {
        jerk = lstepper->max_jerk;
    } else {
        jerk = luaL_checknumber(L, 5);
    }

    #if (STEPPER_DEBUG || STEPPER_STATS)
    syslog(LOG_INFO, "\r\nMove stepper, distance: %.4f units, target speed: %.4f units/s, acceleration: %.4f units/s^2, jerk: %.4f units/s^3, %.4f steps/unit, %.4f unit/step", units, speed, accel, jerk, lstepper->stpu, 1.0 / lstepper->stpu);
    #endif

    float ispd = lstepper->min_spd;

    #if STEPPER_DEBUG
    syslog(LOG_DEBUG, "  Initial speed set to: %.4f units/s", ispd);
    #endif

    // Sanity checks
    if (speed < lstepper->min_spd) {
        speed = lstepper->min_spd;

        #if STEPPER_DEBUG
        syslog(LOG_DEBUG, "  Speed set to min speed: %.4f units/s", speed);
        #endif
    }

    if (speed > lstepper->max_spd) {
        speed = lstepper->max_spd;

        #if STEPPER_DEBUG
        syslog(LOG_DEBUG, "  Target speed set to max speed: %.4f units/s", speed);
        #endif
    }

    if (accel < 0) {
        return luaL_exception(L, STEPPER_ERR_INVALID_ACCELERATION);
    }

    stepper_move(lstepper->unit, units, ispd, speed, accel, jerk);

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
    { LSTRKEY( "attach" ),        LFUNCVAL( lstepper_attach    ) },
    { LSTRKEY( "start"  ),        LFUNCVAL( lstepper_start     ) },
    { LSTRKEY( "stop"   ),        LFUNCVAL( lstepper_stop      ) },
    DRIVER_REGISTER_LUA_ERRORS(stepper)
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lstepper_inst_map[] = {
    { LSTRKEY( "move"        ),   LFUNCVAL( lstepper_move      ) },
    { LSTRKEY( "__metatable" ),   LROVAL  ( lstepper_inst_map  ) },
    { LSTRKEY( "__index"     ),   LROVAL  ( lstepper_inst_map  ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_stepper( lua_State *L ) {
    luaL_newmetarotable(L,"stepper.inst", (void *)lstepper_inst_map);
    return 0;
}

MODULE_REGISTER_ROM(STEPPER, stepper, lstepper_map, luaopen_stepper, 1);

#endif

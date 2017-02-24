/*
 * Lua RTOS, stepper Lua module
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

#include "luartos.h"

#if LUA_USE_STEPPER

#include "lua.h"
#include "lauxlib.h"

#include <drivers/gpio/gpio.h>
#include <drivers/cpu/error.h>
#include <drivers/stepper/stepper.h>

static int setup = 0;
static int lstepper_id = 0;

typedef struct {
    int unit;
} stepper_userdata;

struct lstepp {
    int setup;
    int done;
};

struct lstepp lstepp[NSTEP];

static void lstepper_end(void *arg1, uint32_t mask) {
    int unit;
    struct stepper *p = (struct stepper *)arg1;
    
    for(unit=0;unit < NSTEP; unit++, mask = (mask >> 1)) {
        if (mask & 0b1) {
            lstepp[unit].done = 1;            
        }
    }    
}

static int lstepper_unit() {
    return lstepper_id++;
}

static int lstepper_setup( lua_State* L ) {
	driver_error_t *error;
    int pulse_width = luaL_checkinteger(L, 1); 
    
    if (pulse_width <= 0) {
      return luaL_error(L, "invalid step pulse width");        
    }

    if ((error = steppers_setup(pulse_width, lstepper_end))) {
        return luaL_driver_error(L, "steppers can't setup", error);
    }
    
    setup = 1;
    
    return 0;
}

static int lstepper_new( lua_State* L ){
	driver_error_t *error;
    stepper_userdata *lstepper;
    int port, pin;
    int unit;
    
    int step_pin = luaL_checkinteger(L, 1); 
    int dir_pin = luaL_checkinteger(L, 2); 

    if (!setup) {
      return luaL_error(L, "stepper module is not setup. Setup first.");        
    }
    
    port = PORB(step_pin);
    pin = PINB(step_pin);
    
    if(!platform_pio_has_port(port + 1) || !platform_pio_has_pin(port + 1, pin))
      return luaL_error(L, "invalid step pin");
    
    port = PORB(dir_pin);
    pin = PINB(dir_pin);
    
    if(!platform_pio_has_port(port + 1) || !platform_pio_has_pin(port + 1, pin))
      return luaL_error(L, "invalid dir pin");

    unit = lstepper_unit();
    
    if ((error = stepper_setup(unit + 1, step_pin, dir_pin))) {
        return luaL_driver_error(L, "stepper can't setup", error);
    }

    lstepp[unit].setup = 1;
    lstepp[unit].done = 1;
    
    // Allocate stepper structure and initialize
    lstepper = (stepper_userdata *)lua_newuserdata(L, sizeof(stepper_userdata));
    lstepper->unit = unit;
        
    luaL_getmetatable(L, "stepper");
    lua_setmetatable(L, -2);

    return 1;
}

static int lstepper_move( lua_State* L ){
    stepper_userdata *lstepper = NULL;

    lstepper = (stepper_userdata *)luaL_checkudata(L, 1, "stepper");
    luaL_argcheck(L, lstepper, 1, "stepper expected");

    if (!setup) {
      return luaL_error(L, "stepper module is not setup. Setup first.");        
    }

    int dir =  luaL_checkinteger(L, 2);
    int steps = luaL_checkinteger(L, 3);
    int ramp = luaL_checkinteger(L, 4);
    double ifreq = luaL_checknumber(L, 5);
    double efreq = luaL_checknumber(L, 6);
        
    stepper_move(lstepper->unit + 1, dir, steps, ramp, ifreq, efreq);
    
    return 0;
}

static int lstepper_start( lua_State* L ){
    stepper_userdata *lstepper = NULL;

    int total = lua_gettop(L);
    int mask = 0;
    int i;

    if (!setup) {
      return luaL_error(L, "stepper module is not setup. Setup first.");        
    }
    
    for (i=1; i <= total; i++) {
        if (!lua_isnil(L, i)) {
            lstepper = (stepper_userdata *)luaL_checkudata(L, i, "stepper");
            luaL_argcheck(L, lstepper, i, "stepper expected");
        
            mask |= (1 << (lstepper->unit));

            lstepp[lstepper->unit].done = 0;
        }
    }
    
    stepper_start(mask);
    
    return 0;
}

static int lstepper_done( lua_State* L ){
    stepper_userdata *lstepper = NULL;

    if (!setup) {
      return luaL_error(L, "stepper module is not setup. Setup first.");        
    }

    lstepper = (stepper_userdata *)luaL_checkudata(L, 1, "stepper");
    luaL_argcheck(L, lstepper, 1, "stepper expected");
    
    lua_pushboolean(L, lstepp[lstepper->unit].done);    

    return 1;
}

const luaL_Reg stepper_map[] = {
  { "setup", lstepper_setup },
  { "new", lstepper_new },
  { "move", lstepper_move },
  { "start", lstepper_start },
  { "done", lstepper_done },
  { NULL, NULL }
};

static const luaL_Reg lstepper[] = {
    {NULL, NULL}
};

int luaopen_stepper(lua_State* L)
{
    luaL_newlib(L, lstepper);

    // Set it as its own metatable
    lua_pushvalue( L, -1 );
    lua_setmetatable( L, -2 );

    // create metatable
    luaL_newmetatable(L, "stepper");

    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "cw");

    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "ccw");

    lua_pushliteral(L, "__index");
    lua_pushvalue(L,-2);
    lua_rawset(L,-3);

    // Setup the methods inside metatable
    luaL_register( L, NULL, stepper_map );

    return 1;
}

#endif

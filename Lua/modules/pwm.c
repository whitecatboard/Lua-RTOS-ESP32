/*
 * Whitecat, pwm Lua module
 *
 * Copyright (C) 2015 - 2016
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

#include "whitecat.h"

#if LUA_USE_PWM

#include "lua.h"
#include "lauxlib.h"

#include <drivers/pwm/pwm.h>

struct pwm {
    unsigned char configured;
    unsigned char mode;
    unsigned char res;
    unsigned int  khz;
    unsigned int  started;
};

static struct pwm pwm[NOC];

static int lpwm_pins( lua_State* L ) {
    return platform_pwm_pins(L);
}

static int lpwm_setup(lua_State* L) {
    double duty;
    
    int res, val, khz;
    
    res = 0;
    khz = 0;
    duty = 0;
    val = 0;
    
    int id = luaL_checkinteger(L, 1); 
    int mode = luaL_checkinteger(L, 2); 
    
    if (!platform_pwm_exists(L, id)) {
        return luaL_error(L, "pwm%d does not exist", id);
    }
    
    switch (mode) {
        case 0:
            khz = luaL_checkinteger(L, 3); 
            duty = luaL_checknumber(L, 4); 
            break;
            
        case 1:
            res = luaL_checkinteger(L, 3); 
            val = luaL_checknumber(L, 4); 
            break;
            
        default:
            return luaL_error(L, "invalid setup mode");
    }

    // Check for setup needed
    if (pwm[id - 1].configured && (pwm[id - 1].mode == mode) && (pwm[id - 1].res == res) && (pwm[id - 1].khz == khz)) {
        // No changes
        lua_pushinteger(L, platform_pwm_freq(L, id));
        return 1;
    }
    
    // Setup is needed, if configured, stop first
    if (pwm[id - 1].configured) {
        platform_pwm_stop(L, id);        
    }

    // Configure
    switch (mode) {
        case 0:
            lua_pushinteger(L, platform_pwm_setup_freq(L, id, khz, duty));
            break;
            
        case 1:
            lua_pushinteger(L, platform_pwm_setup_res(L, id, res, val));
            break;
            
        default:
            return luaL_error(L, "invalid setup mode");
    }

    pwm[id - 1].mode = mode;
    pwm[id - 1].res = res;
    pwm[id - 1].khz = khz;
    pwm[id - 1].configured = 1;

    return 1;
}

static int lpwm_down(lua_State* L) {
    int i;
    
    for(i = 0; i < NOC; i++) {
        if (pwm[i].configured) {
            platform_pwm_end(L, i + 1);
        }

        pwm[i].configured = 0;
        pwm[i].started = 0;        
    }

    return 0;
}

static int lpwm_start(lua_State* L) {
    int id = luaL_checkinteger(L, 1); 

    if (!platform_pwm_exists(L, id)) {
        return luaL_error(L, "pwm%d does not exist", id);
    }
    
    if (!pwm[id - 1].configured) {
        return luaL_error(L, "pwm%d is not setup", id);
    }
    
    platform_pwm_start(L, id);
    
    pwm[id - 1].started = 1;
    
    return 0;
}

static int lpwm_stop(lua_State* L) {
    int id = luaL_checkinteger(L, 1); 

    if (!platform_pwm_exists(L, id)) {
        return luaL_error(L, "pwm%d does not exist", id);
    }

    if (!pwm[id - 1].configured) {
        return luaL_error(L, "pwm%d is not setup", id);
    }

    platform_pwm_stop(L, id);

    pwm[id - 1].started = 0;
    
    return 0;
}

static int lpwm_setduty(lua_State* L) {
    int id = luaL_checkinteger(L, 1); 
    double duty = luaL_checknumber(L, 2); 

    if (!platform_pwm_exists(L, id)) {
        return luaL_error(L, "pwm%d does not exist", id);
    }
    
    if (pwm[id - 1].mode != 0) {
        return luaL_error(L, "pwm%d isn't setup in DEFAULT mode, function not allowed", id);
    }

    if (!pwm[id - 1].configured) {
        return luaL_error(L, "pwm%d is not setup", id);
    }

    if (!pwm[id - 1].started) {
        return luaL_error(L, "pwm%d is not started", id);
    }

    platform_pwm_set_duty(L, id, duty);
    
    return 0;
}

static int lpwm_write(lua_State* L) {
    int id = luaL_checkinteger(L, 1); 
    int val = luaL_checknumber(L, 2); 

    if (!platform_pwm_exists(L, id)) {
        return luaL_error(L, "pwm%d does not exist", id);
    }
    
    if (pwm[id - 1].mode != 1) {
        return luaL_error(L, "pwm%d isn't setup in DAC mode, function not allowed", id);
    }

    if (!pwm[id - 1].configured) {
        return luaL_error(L, "pwm%d is not setup", id);
    }

    if (!pwm[id - 1].started) {
        return luaL_error(L, "pwm%d is not started", id);
    }

    platform_pwm_write(L, id, pwm[id - 1].res, val);
    
    return 0;
}

static const luaL_Reg lpwm[] = {
    {"pins", lpwm_pins},
    {"setup", lpwm_setup}, 
    {"down", lpwm_down}, 
    {"start", lpwm_start}, 
    {"stop", lpwm_stop}, 
    {"setduty", lpwm_setduty}, 
    {"write", lpwm_write}, 
    {NULL, NULL}
};

int luaopen_pwm(lua_State* L)
{
    luaL_newlib(L, lpwm);

    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "DEFAULT");

    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "DAC");

    int i;
    char buff[5];

    for(i=1;i<=NOC;i++) {
        if (platform_pwm_exists(L, i)) {
            sprintf(buff,"PWM%d",i);
            lua_pushinteger(L, i);
            lua_setfield(L, -2, buff);
        }
    }

    for(i=0;i<NOC;i++) {
        pwm[i].configured = 0;
        pwm[i].khz = 0;
        pwm[i].mode = 0;
        pwm[i].res = 0;
        pwm[i].started = 0;
    }

    return 1;
}

#endif
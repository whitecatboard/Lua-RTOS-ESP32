/*
 * Whitecat, platform functions for lua PWM module
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
 * whatsoever resulting from loss of use, ƒintedata or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "whitecat.h"

#if LUA_USE_PWM

#include <drivers/pwm/pwm.h>

#include "lua.h"

int platform_pwm_exists(lua_State* L, int id) {
    return ((id > 0) && (id <= NOC) && (id != 6) && (id != 9) && (id != 3));
}

int platform_pwm_pins(lua_State* L) {
    int i;
    unsigned char pin;
    
    for(i=1;i<=NOC;i++) {
        pwm_pins(i, &pin);

        if (pin != 0) {
            printf(
                "pwm%d: %c%d\t(pin %2d)\n", i,
                gpio_portname(pin), gpio_pinno(pin),cpu_pin_number(pin)
            );            
        }
    }
    
    return 0;
}

int platform_pwm_freq(lua_State* L, int id) {
    return pwm_freq(id);
}

int platform_pwm_setup_freq(lua_State* L, int id, int khz, double duty) {
    tdriver_error *error;
    
    // Setup in base of frequency
    error = pwm_init_freq(id, khz, duty);
    if (error) {
        return luaL_driver_error(L, "pwm can't setup", error);
    }
    
    // Return real frequency
    return pwm_freq(id);
}

int platform_pwm_setup_res(lua_State* L, int id, int res, int val) {
    tdriver_error *error;
    
    // Setup in base of resolution
    error = pwm_init_res(id, res, val);
    if (error) {
        return luaL_driver_error(L, "pwm can't setup", error);
    }
    
    // Return real frequency
    return pwm_freq(id);
}

void platform_pwm_start(lua_State* L, int id) {
    pwm_start(id);
}

void platform_pwm_stop(lua_State* L, int id) {
    pwm_stop(id);
}

void platform_pwm_set_duty(lua_State* L, int id, double duty) {
    pwm_set_duty(id, duty);
}

void platform_pwm_write(lua_State* L, int id, int res, int value) {
    pwm_write(id, res, value);
}

void platform_pwm_end(lua_State* L, int id) {
    pwm_end(id);
}

#endif
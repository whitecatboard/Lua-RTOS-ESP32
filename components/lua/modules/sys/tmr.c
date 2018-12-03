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
 * Lua RTOS, Lua timer module
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_TMR

#include "freertos/FreeRTOS.h"
#include "freertos/adds.h"
#include "freertos/timers.h"

#include "esp_attr.h"

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"
#include "error.h"
#include "tmr.h"

#include <string.h>
#include <unistd.h>

#include <sys/delay.h>

#include <drivers/timer.h>
#include <drivers/cpu.h>

typedef lua_callback_t *tmr_callback_t;

static tmr_callback_t callbacks[CPU_LAST_TIMER + 1];
static uint8_t stdio;

static void callback_hw_func(void *arg) {
    int unit = (int)arg;

    // Set standards streams
    if (!stdio) {
        __getreent()->_stdin  = _GLOBAL_REENT->_stdin;
        __getreent()->_stdout = _GLOBAL_REENT->_stdout;
        __getreent()->_stderr = _GLOBAL_REENT->_stderr;

        // Work-around newlib is not compiled with HAVE_BLKSIZE flag
        setvbuf(_GLOBAL_REENT->_stdin , NULL, _IONBF, 0);
        setvbuf(_GLOBAL_REENT->_stdout, NULL, _IONBF, 0);
        setvbuf(_GLOBAL_REENT->_stderr, NULL, _IONBF, 0);

        stdio = 1;
    }

    if (callbacks[unit]) {
        luaS_callback_call(callbacks[unit], 0);
    }
}

static void callback_sw_func(TimerHandle_t xTimer) {
    tmr_userdata *tmr = (tmr_userdata *)pvTimerGetTimerID(xTimer);

    // Set standards streams
    if (!stdio) {
        __getreent()->_stdin  = _GLOBAL_REENT->_stdin;
        __getreent()->_stdout = _GLOBAL_REENT->_stdout;
        __getreent()->_stderr = _GLOBAL_REENT->_stderr;

        // Work-around newlib is not compiled with HAVE_BLKSIZE flag
        setvbuf(_GLOBAL_REENT->_stdin , NULL, _IONBF, 0);
        setvbuf(_GLOBAL_REENT->_stdout, NULL, _IONBF, 0);
        setvbuf(_GLOBAL_REENT->_stderr, NULL, _IONBF, 0);

        stdio = 1;
    }

    if (tmr->callback) {
        luaS_callback_call(tmr->callback, 0);
    }
}

static int ltmr_delay( lua_State* L ) {
    unsigned int period = luaL_checkinteger( L, 1 );
    
    delay(period * 1000);
    
    return 0;
}

static int ltmr_delay_ms( lua_State* L ) {
    unsigned int period = luaL_checkinteger( L, 1 );

    delay(period);
        
    return 0;
}

static int ltmr_delay_us( lua_State* L ) {
    unsigned int period = luaL_checkinteger( L, 1 );

    // Get current counter
    unsigned int start = xthal_get_ccount();

    // Compute how many cycles are needed for the period delay, and discount
    // cycles need by lua vm to invoke this function and cycles needed by lua vm
    // to return function
    int cycles = ((CPU_HZ / 1000000L) * period) - ((start >= L->ci->ccount)?(start - L->ci->ccount):(start + 0xffffffff - L->ci->ccount)) - 47;

    unsigned int now = start;
    while (cycles > 0) {
        start = now;
        now = xthal_get_ccount();
        cycles -= ((now >= start)?(now - start):(now + 0xffffffff - start));
    }

    return 0;
}

static int ltmr_sleep( lua_State* L ) {
    unsigned int period = luaL_checkinteger( L, 1 );
    sleep(period);
    return 0;
}

static int ltmr_sleep_ms( lua_State* L ) {
    unsigned int period = luaL_checkinteger( L, 1 );
    usleep(period * 1000);
    return 0;
}

static int ltmr_sleep_us( lua_State* L ) {
    unsigned int period = luaL_checkinteger( L, 1 );
    usleep(period);
    
    return 0;
}

static int ltmr_hw_attach( lua_State* L ) {
    driver_error_t *error;

    int id = luaL_checkinteger(L, 1);
    uint32_t micros = luaL_checkinteger(L, 2);
    if (micros < 500) {
        return luaL_exception(L, TIMER_ERR_INVALID_PERIOD);
    }

    tmr_userdata *tmr = (tmr_userdata *)lua_newuserdata(L, sizeof(tmr_userdata));
    if (!tmr) {
        return luaL_exception(L, TIMER_ERR_NOT_ENOUGH_MEMORY);
    }

    tmr->type = TmrHW;
    tmr->unit = id;

    // Create the lua callback
    tmr->callback = luaS_callback_create(L, 3);
    if (tmr->callback == NULL) {
        return luaL_exception(L, TIMER_ERR_NOT_ENOUGH_MEMORY);
    }

    callbacks[id] = tmr->callback;

    if ((error = tmr_setup(id, micros, callback_hw_func, 1))) {
        luaS_callback_destroy(tmr->callback);
        callbacks[id] = NULL;
        tmr->callback = NULL;

        return luaL_driver_error(L, error);
    }

    luaL_getmetatable(L, "tmr.timer");
    lua_setmetatable(L, -2);

    return 1;
}

static int ltmr_sw_attach( lua_State* L ) {
    tmr_userdata *tmr = (tmr_userdata *)lua_newuserdata(L, sizeof(tmr_userdata));
    if (!tmr) {
        return luaL_exception(L, TIMER_ERR_NOT_ENOUGH_MEMORY);
    }

    uint32_t millis = luaL_checkinteger(L, 1);
    if (millis < 1) {
        return luaL_exception(L, TIMER_ERR_INVALID_PERIOD);
    }

    tmr->type = TmrSW;

    // Create the lua callback
    tmr->callback = luaS_callback_create(L, 2);
    if (tmr->callback == NULL) {
        return luaL_exception(L, TIMER_ERR_NOT_ENOUGH_MEMORY);
    }

    tmr->h = xTimerCreate("tmr", millis / portTICK_PERIOD_MS, pdTRUE, (void *)tmr, callback_sw_func);
    if (!tmr->h) {
        return luaL_exception(L, TIMER_ERR_NOT_ENOUGH_MEMORY);
    }

    luaL_getmetatable(L, "tmr.timer");
    lua_setmetatable(L, -2);

    return 1;
}

static int ltmr_attach( lua_State* L ) {
    if ((lua_gettop(L) == 3)) {
        return ltmr_hw_attach(L);
    } else {
        return ltmr_sw_attach(L);
    }
}

static int ltmr_start( lua_State* L ) {
    driver_error_t *error;
    tmr_userdata *tmr = NULL;

    tmr = (tmr_userdata *)luaL_checkudata(L, 1, "tmr.timer");
    luaL_argcheck(L, tmr, 1, "tmr expected");

    if (tmr->type == TmrHW) {
        if ((error = tmr_start(tmr->unit))) {
            return luaL_driver_error(L, error);
        }
    } else if (tmr->type == TmrSW) {
        xTimerStart(tmr->h, 0);
    }

    return 0;
}

static int ltmr_stop( lua_State* L ) {
    driver_error_t *error;
    tmr_userdata *tmr = NULL;

    tmr = (tmr_userdata *)luaL_checkudata(L, 1, "tmr.timer");
    luaL_argcheck(L, tmr, 1, "tmr expected");

    if (tmr->type == TmrHW) {
        if ((error = tmr_stop(tmr->unit))) {
            return luaL_driver_error(L, error);
        }
    } else if (tmr->type == TmrSW) {
        xTimerStop(tmr->h, 0);
    }

    return 0;
}

static int ltmr_detach (lua_State *L) {
    tmr_userdata *tmr = NULL;
    tmr = (tmr_userdata *)luaL_checkudata(L, 1, "tmr.timer");

    if (tmr->type == TmrHW) {
        tmr_ll_unsetup(tmr->unit);
        callbacks[tmr->unit] = NULL;
    } else if (tmr->type == TmrSW) {
        xTimerStop(tmr->h, portMAX_DELAY);
        xTimerDelete(tmr->h, portMAX_DELAY);
    }

    // Destroy callback
    if (tmr->callback) {
        luaS_callback_destroy(tmr->callback);
    }

    memset(tmr, 0, sizeof(tmr_userdata));

    return 0;
}

// Destructor
static int ltmr_gc (lua_State *L) {
    ltmr_detach(L);

    return 0;
}

static const LUA_REG_TYPE tmr_timer_map[] = {
    { LSTRKEY( "start"       ),     LFUNCVAL( ltmr_start    ) },
    { LSTRKEY( "stop"        ),     LFUNCVAL( ltmr_stop     ) },
    { LSTRKEY( "detach"      ),     LFUNCVAL( ltmr_detach   ) },
    { LSTRKEY( "__metatable" ),     LROVAL  ( tmr_timer_map ) },
    { LSTRKEY( "__index"     ),     LROVAL  ( tmr_timer_map ) },
    { LSTRKEY( "__gc"        ),     LFUNCVAL( ltmr_gc       ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE tmr_map[] = {
    { LSTRKEY( "attach"      ),     LFUNCVAL( ltmr_attach   ) },
    { LSTRKEY( "delay"       ),     LFUNCVAL( ltmr_delay    ) },
    { LSTRKEY( "delayms"     ),     LFUNCVAL( ltmr_delay_ms ) },
    { LSTRKEY( "delayus"     ),     LFUNCVAL( ltmr_delay_us ) },
    { LSTRKEY( "sleep"       ),     LFUNCVAL( ltmr_sleep    ) },
    { LSTRKEY( "sleepms"     ),     LFUNCVAL( ltmr_sleep_ms ) },
    { LSTRKEY( "sleepus"     ),     LFUNCVAL( ltmr_sleep_us ) },
    TMR_TMR0
    TMR_TMR1
    TMR_TMR2
    TMR_TMR3
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_tmr( lua_State *L ) {
    memset(callbacks, 0, sizeof(callbacks));

    luaL_newmetarotable(L,"tmr.timer", (void *)tmr_timer_map);
    return 0;
}

MODULE_REGISTER_ROM(TMR, tmr, tmr_map, luaopen_tmr, 1);

/*

function blink()
    if (led_on) then
        pio.pin.sethigh(pio.GPIO26)
        led_on = false
    else
        pio.pin.setlow(pio.GPIO26)
        led_on = true
    end
end

led_on = false
pio.pin.setdir(pio.OUTPUT, pio.GPIO26)

t0 = tmr.attach(tmr.TMR0, 50000, blink)
t0:start()
t0:stop()

---

function blink()
    if (led_on) then
        pio.pin.sethigh(pio.GPIO26)
        led_on = false
    else
        pio.pin.setlow(pio.GPIO26)
        led_on = true
    end
end

led_on = false
pio.pin.setdir(pio.OUTPUT, pio.GPIO26)

s0 = tmr.attach(50000, blink)
s0:start()
s0:stop()


 */
#endif

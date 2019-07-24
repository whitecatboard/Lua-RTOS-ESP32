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
 * Lua RTOS, Lua event module
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_EVENT

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "error.h"
#include "event.h"
#include "modules.h"

#include <string.h>

#include <sys/driver.h>

#include <pthread.h>

#define EVENT_ERR_NOT_ENOUGH_MEMORY (DRIVER_EXCEPTION_BASE(EVENT_DRIVER_ID) |  0)

// Register driver and messages
DRIVER_REGISTER_BEGIN(EVENT,event,0,NULL,NULL);
    DRIVER_REGISTER_ERROR(EVENT, event, NotEnoughtMemory, "not enough memory", EVENT_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_END(EVENT,event,0,NULL,NULL);

static int has_pending_events(event_userdata_t *udata) {
    int pending;

    mtx_lock(&udata->mtx);
    pending = (udata->pending > 0);
    mtx_unlock(&udata->mtx);

    return pending;
}

/*
 * Get the listener that corresponds to the current thread. If current thread has not
 * a listener, create it.
 */
static int get_listener(lua_State* L, event_userdata_t *udata, listener_data_t **listener_data) {
    listener_data_t *clistener;

    *listener_data = NULL;

    // Get the current thread
    pthread_t thread = pthread_self();

    mtx_lock(&udata->mtx);

    // Search for listener
    int idx = 1;
    int listener_id = 0;
    while (idx >= 1) {
        if (lstget(&udata->listeners, idx, (void **)&clistener)) {
            break;
        }

        if (clistener->thread == thread) {
            listener_id = idx;
            break;
        }

        // Next listener
        idx = lstnext(&udata->listeners, idx);
    }

    // If not found, create a new listener for the current thread
    if (!listener_id) {
        // Listener not found
        // Allocate space for the new listener
        clistener = (listener_data_t *)calloc(1,sizeof(listener_data_t));
        if (!clistener) {
            mtx_unlock(&udata->mtx);
            return luaL_exception(L, EVENT_ERR_NOT_ENOUGH_MEMORY);
        }

        // Create a queue for the listener
        clistener->q = xQueueCreate(1, sizeof(uint8_t));
        if (!clistener->q) {
            free(clistener);
            mtx_unlock(&udata->mtx);
            return luaL_exception(L, EVENT_ERR_NOT_ENOUGH_MEMORY);
        }

        clistener->thread = thread;

        //Add listener data
        int id;

        if (lstadd(&udata->listeners, clistener, &id)) {
            free(clistener);
            mtx_unlock(&udata->mtx);
            return luaL_exception(L, EVENT_ERR_NOT_ENOUGH_MEMORY);
        }
    } else {
        // Listener found
        lstget(&udata->listeners, listener_id, (void **)&clistener);
    }

    *listener_data = clistener;

    mtx_unlock(&udata->mtx);

    return 0;
}

/*
 * Remove the listener that corresponds to the current thread.
 */
static int remove_listener(lua_State* L, event_userdata_t *udata) {
    listener_data_t *clistener;

    // Get the current thread
    pthread_t thread = pthread_self();

    mtx_lock(&udata->mtx);

    // Search for listener
    int idx = 1;
    int listener_id = 0;
    while (idx >= 1) {
        if (lstget(&udata->listeners, idx, (void **)&clistener)) {
            break;
        }

        if (clistener->thread == thread) {
            listener_id = idx;
            break;
        }

        // Next listener
        idx = lstnext(&udata->listeners, idx);
    }

    // If found, remove
    if (listener_id) {
        vQueueDelete((xQueueHandle)clistener->q);
        lstremove(&udata->listeners, listener_id, 1);
    }

    mtx_unlock(&udata->mtx);

    return 0;
}

static int levent_create( lua_State* L ) {
    // Create user data
    event_userdata_t *udata = (event_userdata_t *)lua_newuserdata(L, sizeof(event_userdata_t));
    if (!udata) {
        return luaL_exception(L, EVENT_ERR_NOT_ENOUGH_MEMORY);
    }

    memset(udata,0, sizeof(event_userdata_t));

    // Init mutex
    mtx_init(&udata->mtx, NULL, NULL, 0);

    // Create the listener list
    lstinit(&udata->listeners, 1, LIST_DEFAULT);

    // Create a queue for sync this event with the termination of the
    // listeners, when using broadcast(true)
    udata->q = xQueueCreate(10, sizeof(uint8_t));
    if (!udata->q) {
        mtx_destroy(&udata->mtx);
        free(udata);
        return luaL_exception(L, EVENT_ERR_NOT_ENOUGH_MEMORY);
    }

    luaL_getmetatable(L, "event.ins");
    lua_setmetatable(L, -2);

    return 1;
}

static int levent_enable( lua_State* L ) {
    event_userdata_t *udata = NULL;

    // Get user data
    udata = (event_userdata_t *)luaL_checkudata(L, 1, "event.ins");
    luaL_argcheck(L, udata, 1, "event expected");

    mtx_lock(&udata->mtx);
    udata->disabled = 0;
    mtx_unlock(&udata->mtx);

    return 0;
}

static int levent_disable( lua_State* L ) {
    event_userdata_t *udata = NULL;
    listener_data_t *listener_data;

    // Get user data
    udata = (event_userdata_t *)luaL_checkudata(L, 1, "event.ins");
    luaL_argcheck(L, udata, 1, "event expected");

    mtx_lock(&udata->mtx);

    udata->disabled = 1;

    // Unblock threads that are waiting for this event
    int idx = 1;

    while (idx >= 1) {
        // Get queue
        if (lstget(&udata->listeners, idx, (void **)&listener_data)) {
            break;
        }

        listener_data->is_waiting = 1;
        udata->pending++;

        mtx_unlock(&udata->mtx);

        // Unblock
        uint8_t d = 1;
        xQueueSend((xQueueHandle)listener_data->q, &d, portMAX_DELAY);

        mtx_lock(&udata->mtx);

        // Next listener
        idx = lstnext(&udata->listeners, idx);
    }

    mtx_unlock(&udata->mtx);

    // Wait for the termination of all threads
    uint8_t d;
    while (has_pending_events(udata)) {
        xQueueReceive(udata->q, &d, portMAX_DELAY);

        mtx_lock(&udata->mtx);
        udata->pending--;
        mtx_unlock(&udata->mtx);
    }

    return 0;
}

static int levent_wait( lua_State* L ) {
    event_userdata_t *udata = NULL;
    listener_data_t *clistener;

    // Get user data
    udata = (event_userdata_t *)luaL_checkudata(L, 1, "event.ins");
    luaL_argcheck(L, udata, 1, "event expected");

    mtx_lock(&udata->mtx);
    if (udata->disabled) {
        lua_pushboolean(L, 0);
        mtx_unlock(&udata->mtx);
        return 1;
    }
    mtx_unlock(&udata->mtx);

    // Get an existing listener for the current thread
    get_listener(L, udata, &clistener);

    // Wait for broadcast
    uint8_t d = 0;
    xQueueReceive(clistener->q, &d, portMAX_DELAY);

    lua_pushboolean(L, (d == 0));

    return 1;
}

static int levent_done( lua_State* L ) {
    event_userdata_t *udata = NULL;
    listener_data_t *listener_data ;

    // Get user data
    udata = (event_userdata_t *)luaL_checkudata(L, 1, "event.ins");
    luaL_argcheck(L, udata, 1, "event expected");

    // Get an existing listener for the current thread
    get_listener(L, udata, &listener_data);

    mtx_lock(&udata->mtx);

    if (listener_data->is_waiting) {
        mtx_unlock(&udata->mtx);

        uint8_t d = 0;
        xQueueSend((xQueueHandle)udata->q, &d, portMAX_DELAY);

        mtx_lock(&udata->mtx);
    }

    mtx_unlock(&udata->mtx);

    remove_listener(L, udata);

    return 0;
}

static int levent_pending( lua_State* L ) {
    event_userdata_t *udata = NULL;

    // Get user data
    udata = (event_userdata_t *)luaL_checkudata(L, 1, "event.ins");
    luaL_argcheck(L, udata, 1, "event expected");

    lua_pushboolean(L, has_pending_events(udata));

    return 1;
}

static int levent_broadcast( lua_State* L ) {
    event_userdata_t *udata = NULL;
    listener_data_t *clistener;

    // Get user data
    udata = (event_userdata_t *)luaL_checkudata(L, 1, "event.ins");
    luaL_argcheck(L, udata, 1, "event expected");

    // Check for wait
    uint8_t wait = 0;
    if (lua_gettop(L) == 2) {
        luaL_checktype(L, 2, LUA_TBOOLEAN);
        wait = lua_toboolean(L, 2);
    }

    mtx_lock(&udata->mtx);

    if (udata->disabled) {
        lua_pushboolean(L, 0);
        mtx_unlock(&udata->mtx);
        return 1;
    }

    // Unblock threads that are waiting for this event
    int idx = 1;

    while (idx >= 1) {
        // Get queue
        if (lstget(&udata->listeners, idx, (void **)&clistener)) {
            break;
        }

        clistener->is_waiting = wait;

        if (wait) {
            udata->pending++;
        }

        mtx_unlock(&udata->mtx);

        // Unblock
        uint8_t d = 0;
        xQueueSend((xQueueHandle)clistener->q, &d, portMAX_DELAY);

        mtx_lock(&udata->mtx);

        // Next listener
        idx = lstnext(&udata->listeners, idx);
    }

    mtx_unlock(&udata->mtx);

    if (wait) {
        // If broadcast with waiting, wait for the termination of all threads
        uint8_t d;
        while (has_pending_events(udata)) {
            xQueueReceive(udata->q, &d, portMAX_DELAY);

            mtx_lock(&udata->mtx);
            udata->pending--;
            mtx_unlock(&udata->mtx);
        }
    }

    lua_pushboolean(L, 1);
    mtx_unlock(&udata->mtx);
    return 1;
}

// Destructor
static int levent_ins_gc (lua_State *L) {
    event_userdata_t *udata = NULL;
    listener_data_t *listener_data;
    int res;
    int idx = 1;

    udata = (event_userdata_t *)luaL_checkudata(L, 1, "event.ins");
    if (udata) {
        // Destroy all listeners
        while (idx >= 0) {
            res = lstget(&udata->listeners, idx, (void **)&listener_data);
            if (res) {
                break;
            }

            vQueueDelete((xQueueHandle)listener_data->q);

            lstremove(&udata->listeners, idx, 1);
            idx = lstnext(&udata->listeners, idx);
        }

        vQueueDelete((xQueueHandle)udata->q);

        mtx_destroy(&udata->mtx);
        lstdestroy(&udata->listeners, 0);
    }

    return 0;
}

static const LUA_REG_TYPE levent_map[] = {
    { LSTRKEY( "create"  ),            LFUNCVAL( levent_create   ) },
    DRIVER_REGISTER_LUA_ERRORS(event)
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE levent_ins_map[] = {
    { LSTRKEY( "wait"        ),        LFUNCVAL( levent_wait        ) },
    { LSTRKEY( "done"        ),        LFUNCVAL( levent_done        ) },
    { LSTRKEY( "disable"     ),     LFUNCVAL( levent_disable     ) },
    { LSTRKEY( "enable"      ),        LFUNCVAL( levent_enable      ) },
    { LSTRKEY( "pending"     ),        LFUNCVAL( levent_pending     ) },
      { LSTRKEY( "broadcast"   ),        LFUNCVAL( levent_broadcast   ) },
    { LSTRKEY( "__metatable" ),        LROVAL  ( levent_ins_map     ) },
    { LSTRKEY( "__index"     ),       LROVAL  ( levent_ins_map     ) },
    { LSTRKEY( "__gc"        ),       LFUNCVAL( levent_ins_gc      ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_event( lua_State *L ) {
    luaL_newmetarotable(L,"event.ins", (void*)levent_ins_map);
    return 0;
}

MODULE_REGISTER_ROM(EVENT, event, levent_map, luaopen_event, 1);

#endif


/*

e1 = event.create()
e2 = event.create()

thread.start(function()
  while true do
        e1:wait()
      tmr.delay(2)
      print("hi from 1")
      e2:broadcast()
  end
end)

thread.start(function()
  while true do
        e2:wait()
        tmr.delay(4)
      print("hi from 2")
        e1:broadcast()
  end
end)

e1:broadcast(false)
e2:broadcast(false)

----

e1 = event.create()

thread.start(function()
  while true do
        e1:wait()
      tmr.delay(2)
      print("hi from 1")
      e1:done()
  end
end)

thread.start(function()
  while true do
        e1:wait()
      tmr.delay(4)
      print("hi from 2")
      e1:done()
  end
end)

e1:broadcast(true)

*/

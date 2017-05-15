/*
 * Lua RTOS, Lua event module
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

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_EVENT

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"
#include "event.h"
#include "modules.h"

#include <string.h>

#include <sys/driver.h>

#include <pthread/pthread.h>

// This variables are defined at linker time
extern LUA_REG_TYPE event_error_map[];

// Module errors

#define EVENT_ERR_NOT_ENOUGH_MEMORY (DRIVER_EXCEPTION_BASE(EVENT_DRIVER_ID) |  0)

DRIVER_REGISTER_ERROR(EVENT, event, NotEnoughtMemory, "not enough memory", EVENT_ERR_NOT_ENOUGH_MEMORY);

/*
 * Get the listener that corresponds to the current thread. If current thread has not
 * a listener, create it.
 */
static int get_listener(lua_State* L, event_userdata_t *udata, listener_data_t **listener_data) {
	listener_data_t *clistener;

	*listener_data = NULL;

    // Get the current thread
    pthread_t thread = pthread_self();

    // Search for listener
    int idx = 1;
    int listener_id = 0;
    while (idx >= 1) {
        if (list_get(&udata->listeners, idx, (void **)&clistener)) {
        	break;
        }

        if (clistener->thread == thread) {
        	listener_id = idx;
        	break;
        }

        // Next listener
        idx = list_next(&udata->listeners, idx);
    }

	// If not found, create a new listener for the current thread
    if (!listener_id) {
    	// Listener not found
    	// Allocate space for the new listener
    	clistener = (listener_data_t *)calloc(1,sizeof(listener_data_t));
    	if (!clistener) {
    		return luaL_exception(L, EVENT_ERR_NOT_ENOUGH_MEMORY);
    	}

		mtx_lock(&udata->mtx);

		// Create a queue for the listener
		clistener->q = xQueueCreate(1, sizeof(uint8_t));
		if (!clistener->q) {
			free(clistener);
			mtx_unlock(&udata->mtx);
			return luaL_exception(L, EVENT_ERR_NOT_ENOUGH_MEMORY);
		}

		clistener->thread = pthread_self();

		//Add listener data
		int id;

		if (list_add(&udata->listeners, clistener, &id)) {
			free(clistener);
			mtx_unlock(&udata->mtx);
			return luaL_exception(L, EVENT_ERR_NOT_ENOUGH_MEMORY);
		}

		mtx_unlock(&udata->mtx);
    } else {
    	// Listener found
        list_get(&udata->listeners, listener_id, (void **)&clistener);
    }

    *listener_data = clistener;

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

	mtx_lock(&udata->mtx);

    // Create the listener list
    list_init(&udata->listeners, 1);

    // Create a queue for sync this event with the termination of the
    // listeners, when using broadcast(true)
    udata->q = xQueueCreate(10, sizeof(uint8_t));
	if (!udata->q) {
		free(udata);
		return luaL_exception(L, EVENT_ERR_NOT_ENOUGH_MEMORY);
	}

	mtx_unlock(&udata->mtx);

    luaL_getmetatable(L, "event.ins");
    lua_setmetatable(L, -2);

    return 1;
}

static int levent_wait( lua_State* L ) {
    event_userdata_t *udata = NULL;
    listener_data_t *listener_data;

    // Get user data
    udata = (event_userdata_t *)luaL_checkudata(L, 1, "event.ins");
    luaL_argcheck(L, udata, 1, "event expected");

    // Get an existing listener for the current thread
    int listener_id = get_listener(L, udata, &listener_data);
    if (listener_id) {
    	return listener_id;
    }

    // Wait for broadcast
    uint8_t d = 0;
    xQueueReceive(listener_data->q, &d, portMAX_DELAY);

    return 0;
}

static int levent_done( lua_State* L ) {
    event_userdata_t *udata = NULL;
    listener_data_t *listener_data ;

    // Get user data
    udata = (event_userdata_t *)luaL_checkudata(L, 1, "event.ins");
    luaL_argcheck(L, udata, 1, "event expected");

    // Get an existing listener for the current thread
    int listener_id = get_listener(L, udata, &listener_data);
    if (listener_id) {
    	return listener_id;
    }

	if (listener_data->is_waiting) {
		uint8_t d = 0;
		xQueueSend((xQueueHandle)udata->q, &d, portMAX_DELAY);
	}

	mtx_lock(&udata->mtx);
	udata->pending--;
	mtx_unlock(&udata->mtx);

	return 0;
}

static int levent_pending( lua_State* L ) {
    event_userdata_t *udata = NULL;

    // Get user data
    udata = (event_userdata_t *)luaL_checkudata(L, 1, "event.ins");
    luaL_argcheck(L, udata, 1, "event expected");

	mtx_lock(&udata->mtx);
	int pending = udata->pending;
	mtx_unlock(&udata->mtx);

	lua_pushboolean(L, pending > 0);

	return 1;
}

static int levent_broadcast( lua_State* L ) {
    event_userdata_t *udata = NULL;
    listener_data_t *listener_data;

	// Get user data
	udata = (event_userdata_t *)luaL_checkudata(L, 1, "event.ins");
    luaL_argcheck(L, udata, 1, "event expected");

    // Check for wait
    uint8_t wait = 0;
	if (lua_gettop(L) == 2) {
		luaL_checktype(L, 2, LUA_TBOOLEAN);
		if (lua_toboolean(L, 2)) {
			wait = 1;
		}
	}

	mtx_lock(&udata->mtx);

	// Unblock threads that are waiting for this event
    int idx = 1;

    while (idx >= 1) {
    	// Get queue
        if (list_get(&udata->listeners, idx, (void **)&listener_data)) {
        	break;
        }

        listener_data->is_waiting = wait;
        udata->pending++;

        // Unblock
    	uint8_t d = 0;
    	xQueueSend((xQueueHandle)listener_data->q, &d, portMAX_DELAY);

        if (wait) {
    		// Increment waiting count
    		udata->waiting_for++;
    	}

        // Next listener
        idx = list_next(&udata->listeners, idx);
    }

    if (wait) {
    	// If broadcast with waiting, wait for the termination of all threads
    	uint8_t d;
    	while (udata->waiting_for > 0) {
    	    xQueueReceive(udata->q, &d, portMAX_DELAY);
    	    udata->waiting_for--;
    	}
    }

    mtx_unlock(&udata->mtx);

    return 0;
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
	        res = list_get(&udata->listeners, idx, (void **)&listener_data);
	        if (res) {
	        	break;
	        }

	        vQueueDelete((xQueueHandle)listener_data->q);

	        list_remove(&udata->listeners, idx, 0);
	        idx = list_next(&udata->listeners, idx);
	    }

	    vQueueDelete((xQueueHandle)udata->q);

	    mtx_destroy(&udata->mtx);
	    list_destroy(&udata->listeners, 0);
	}

	return 0;
}

static const LUA_REG_TYPE levent_map[] = {
    { LSTRKEY( "create"  ),			LFUNCVAL( levent_create   ) },
	{ LSTRKEY( "error"   ), 		LROVAL  ( event_error_map )},
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE levent_ins_map[] = {
	{ LSTRKEY( "wait"        ),		LFUNCVAL( levent_wait        ) },
	{ LSTRKEY( "done"        ),		LFUNCVAL( levent_done        ) },
	{ LSTRKEY( "pending"     ),		LFUNCVAL( levent_pending     ) },
  	{ LSTRKEY( "broadcast"   ),		LFUNCVAL( levent_broadcast   ) },
	{ LSTRKEY( "__metatable" ),    	LROVAL  ( levent_ins_map     ) },
	{ LSTRKEY( "__index"     ),   	LROVAL  ( levent_ins_map     ) },
	{ LSTRKEY( "__gc"        ),   	LROVAL  ( levent_ins_gc      ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_event( lua_State *L ) {
    luaL_newmetarotable(L,"event.ins", (void*)levent_ins_map);
    return 0;
}

MODULE_REGISTER_MAPPED(EVENT, event, levent_map, luaopen_event);
DRIVER_REGISTER(EVENT,event,NULL,NULL,NULL);

#endif


/*

e1 = event.create()
e2 = event.create()

thread.start(function()
  while true do
  	  e1:wait()
	  tmr.delay(2)
	  print("hi from 1")
  end
end)

thread.start(function()
  while true do
  	  e2:wait()
  	  e1:broadcast()
  	  tmr.delay(4)
	  print("hi from 2")
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

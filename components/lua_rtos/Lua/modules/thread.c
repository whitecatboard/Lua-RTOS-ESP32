/*
 * Lua RTOS, pthread wrapper for FreeRTOS
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

#if LUA_USE_THREAD

#include "lua.h"
#include "lapi.h"
#include "lauxlib.h"
#include "lgc.h"
#include "lmem.h"
#include "ldo.h"
#include "thread.h"
#include "error.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

#include <pthread.h>

// Module errors
#define LUA_THREAD_ERR_NOT_ENOUGH_MEMORY    (DRIVER_EXCEPTION_BASE(THREAD_DRIVER_ID) |  0)
#define LUA_THREAD_ERR_NOT_ALLOWED          (DRIVER_EXCEPTION_BASE(THREAD_DRIVER_ID) |  1)
#define LUA_THREAD_ERR_NON_EXISTENT         (DRIVER_EXCEPTION_BASE(THREAD_DRIVER_ID) |  2)
#define LUA_THREAD_ERR_CANNOT_START         (DRIVER_EXCEPTION_BASE(THREAD_DRIVER_ID) |  3)
#define LUA_THREAD_ERR_INVALID_STACK_SIZE   (DRIVER_EXCEPTION_BASE(THREAD_DRIVER_ID) |  4)
#define LUA_THREAD_ERR_INVALID_PRIORITY     (DRIVER_EXCEPTION_BASE(THREAD_DRIVER_ID) |  5)
#define LUA_THREAD_ERR_INVALID_CPU_AFFINITY (DRIVER_EXCEPTION_BASE(THREAD_DRIVER_ID) |  6)

DRIVER_REGISTER_ERROR(THREAD, thread, NotEnoughtMemory, "not enough memory", LUA_THREAD_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_ERROR(THREAD, thread, NotAllowed, "not allowed", LUA_THREAD_ERR_NOT_ALLOWED);
DRIVER_REGISTER_ERROR(THREAD, thread, NonExistentThread, "non-existent thread", LUA_THREAD_ERR_NON_EXISTENT);
DRIVER_REGISTER_ERROR(THREAD, thread, CannotStart, "cannot start thread", LUA_THREAD_ERR_CANNOT_START);
DRIVER_REGISTER_ERROR(THREAD, thread, InvalidStackSize, "invalid stack size", LUA_THREAD_ERR_INVALID_STACK_SIZE);
DRIVER_REGISTER_ERROR(THREAD, thread, InvalidPriority, "invalid priority", LUA_THREAD_ERR_INVALID_PRIORITY);
DRIVER_REGISTER_ERROR(THREAD, thread, InvaludCPUAffinity, "invalid CPU affinity", LUA_THREAD_ERR_INVALID_CPU_AFFINITY);

#define LTHREAD_STATUS_RUNNING   1
#define LTHREAD_STATUS_SUSPENDED 2

// List of threads
static struct list lthread_list;

void thread_terminated(void *args) {
    struct lthread *thread;

    int *thid = (void *)args;
    int res = list_get(&lthread_list, *thid, (void **)&thread);
    if (!res) {  
        luaL_unref(thread->PL, LUA_REGISTRYINDEX, thread->function_ref);
        luaL_unref(thread->PL, LUA_REGISTRYINDEX, thread->thread_ref);
            
        list_remove(&lthread_list, *thid, 1);
    }

	// Delay a number of ticks for take the iddle
	// task an opportunity for free allocated memory
	vTaskDelay(10);	
}

void *thread_start_task(void *arg) {
    struct lthread *thread;
    int *thid;
    thread = (struct lthread *)arg;
    
    luaL_checktype(thread->L, 1, LUA_TFUNCTION);
    
    thid = malloc(sizeof(int));
    *thid = thread->thid;
    pthread_cleanup_push(thread_terminated, thid);

    int status = lua_pcall(thread->L, 0, 0, 0);
    if (status != LUA_OK) {
        int *tmp;
        
        tmp = malloc(sizeof(int));
        *tmp = status;
        
        return tmp;
    }
    
    return NULL;
}

static int thread_suspend_pthreads(lua_State *L, int thid) {
    struct lthread *thread;
    int res, idx;
    
    // For now this is not allowd in ESP32
    return luaL_exception_extended(L, LUA_THREAD_ERR_NOT_ALLOWED, "in ESP32");

    if (thid) {
        idx = thid;
    } else {
        idx = list_first(&lthread_list);
    }
    
    while (idx >= 0) {
        res = list_get(&lthread_list, idx, (void **)&thread);
        if (res) {
        	luaL_exception(L, LUA_THREAD_ERR_NON_EXISTENT);
        }

        // If thread is created, suspend
        if (thread->thread) {
            _pthread_suspend(thread->thread);

            // Update thread status
            thread->status = LTHREAD_STATUS_SUSPENDED;
        }

        if (!thid) {
            idx = list_next(&lthread_list, idx);
        } else {
            idx = -1;
        }
    }  
    
    return 0;
}

static int thread_resume_pthreads(lua_State *L, int thid) {
    struct lthread *thread;
    int res, idx;
    
    // For now this is not allowd in ESP32
    return luaL_exception_extended(L, LUA_THREAD_ERR_NOT_ALLOWED, "in ESP32");

    if (thid) {
        idx = thid;
    } else {
        idx = list_first(&lthread_list);
    }
    
    while (idx >= 0) {
        res = list_get(&lthread_list, idx, (void **)&thread);
        if (res) {
        	luaL_exception(L, LUA_THREAD_ERR_NON_EXISTENT);
        }

        // If thread is created, suspend
        if (thread->thread) {
            _pthread_resume(thread->thread);

            // Update thread status
            thread->status = LTHREAD_STATUS_RUNNING;
        }

        if (!thid) {
            idx = list_next(&lthread_list, idx);
        } else {
            idx = -1;
        }
    }  
    
    return 0;
}

static int thread_stop_pthreads(lua_State *L, int thid) {
    struct lthread *thread;
    int res, idx;
    
    if (thid) {
        idx = thid;
    } else {
        idx = list_first(&lthread_list);
    }
    
    while (idx >= 0) {
        res = list_get(&lthread_list, idx, (void **)&thread);
        if (res) {
        	luaL_exception(L, LUA_THREAD_ERR_NON_EXISTENT);
        }

        // If thread is created, suspend
        if (thread->thread) {
            _pthread_stop(thread->thread);            
            _pthread_free(thread->thread);

            luaL_unref(L, LUA_REGISTRYINDEX, thread->function_ref);
            luaL_unref(L, LUA_REGISTRYINDEX, thread->thread_ref);

            list_remove(&lthread_list, idx, 1);
        }

        if (!thid) {
            idx = list_next(&lthread_list, idx);
        } else {
            idx = -1;
        }
    }  
    
    if (!thid) {
    	#if LUA_USE_PWM
        lua_getglobal(L, "pwm"); 
        lua_getfield(L, -1, "down");
        lua_remove(L, -2);
        lua_pcall(L, 0, 0, 0);
        #endif 
    }
    
	// Do a garbage collection
	//lua_lock(L);
	//luaC_fullgc(L, 1);
	//lua_unlock(L);

	// Delay a number of ticks for take the iddle
	// task an opportunity for free allocated memory
	//vTaskDelay(10);
	
    return 0;
}

static int thread_list(lua_State *L) {
    struct lthread *thread;
    int idx;
    char status[5];

    const char *format = luaL_optstring(L, 1, "");

    if (strcmp(format,"*n") == 0) {
        // List only number of current threads
        int n= 0;

        idx = list_first(&lthread_list);
        while (idx >= 0) {
            n++;
            idx = list_next(&lthread_list, idx);
        }       
        
        lua_pushinteger(L, n);
        return 1;
    } else {
        printf("THID\tNAME\t\tSTATUS\tCORE\tTIME\n");

        // For each lthread in list ...
        idx = list_first(&lthread_list);
        while (idx >= 0) {
            list_get(&lthread_list, idx, (void **)&thread);

            // Get status
            switch (thread->status) {
                case LTHREAD_STATUS_RUNNING: strcpy(status,"run "); break;
                case LTHREAD_STATUS_SUSPENDED: strcpy(status,"susp"); break;
                default:
                    strcpy(status,"----");

            }

            printf("%d\t%s\t\t%s\t%d\t%d\n", idx, "", status, _pthread_core(thread->thread),0);

            idx = list_next(&lthread_list, idx);
        }        
    }
    
    return 0;
}

static int new_thread(lua_State* L, int run) {
    struct lthread *thread;
    pthread_attr_t attr;
	struct sched_param sched;
    int res, idx;
    pthread_t id;
    int retries;

    // Get stack size, priotity and cpu affinity
    int stack = luaL_optinteger(L, 2, CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE);
    int priority = luaL_optinteger(L, 3, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY);
    int affinity = luaL_optinteger(L, 4, CONFIG_LUA_RTOS_LUA_THREAD_CPU);

    // Sanity checks
    if (stack < PTHREAD_STACK_MIN) {
    	return luaL_exception(L, LUA_THREAD_ERR_INVALID_STACK_SIZE);
    }

    if ((priority < ESP_TASK_PRIO_MIN + 3) || (priority > ESP_TASK_PRIO_MAX)) {
    	return luaL_exception(L, LUA_THREAD_ERR_INVALID_PRIORITY);
    }

    if ((affinity < 0) || (affinity > 1)) {
    	return luaL_exception(L, LUA_THREAD_ERR_INVALID_CPU_AFFINITY);
    }

    // TO DO
    // Something is wrong somewhere. We need to do this for now.
    while (lua_gettop(L) > 1) {
        lua_remove(L, -1);
    }

    // Allocate space for lthread info
    thread = (struct lthread *)malloc(sizeof(struct lthread));
    if (!thread) {
    	return luaL_exception(L, LUA_THREAD_ERR_NOT_ENOUGH_MEMORY);
    }
    
    // Check for argument is a function, and store it's reference
    luaL_checktype(L, 1, LUA_TFUNCTION);
    thread->function_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    // Create a new state, move function to it and store thread reference
    thread->PL = L;
    thread->L = lua_newthread(L);
    thread->thread_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    thread->status = LTHREAD_STATUS_SUSPENDED;
    
    lua_rawgeti(L, LUA_REGISTRYINDEX, thread->function_ref);                
    lua_xmove(L, thread->L, 1);

    // Add lthread to list
    res = list_add(&lthread_list, thread, &idx);
    if (res) {
        free(thread);
    	return luaL_exception(L, LUA_THREAD_ERR_NOT_ENOUGH_MEMORY);
    }

	// Init thread attributes
	pthread_attr_init(&attr);

	// Set stack size
    pthread_attr_setstacksize(&attr, stack);

    // Set priority
    sched.sched_priority = priority;
    pthread_attr_setschedparam(&attr, &sched);

    // Set CPU
    cpu_set_t cpu_set = affinity;
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);


    if (run)  {
        pthread_attr_setinitialstate(&attr, PTHREAD_INITIAL_STATE_RUN);
    } else {
        pthread_attr_setinitialstate(&attr, PTHREAD_INITIAL_STATE_SUSPEND);        
    }
    
    thread->thid = idx;
    
    retries = 0;
    
retry:  
    res = pthread_create(&id, &attr, thread_start_task, thread);
    if (res) {
        if ((res == ENOMEM) && (retries < 4)) {
            luaC_checkGC(L);  /* stack grow uses memory */
            luaD_checkstack(L, LUA_MINSTACK);  /* ensure minimum stack size */
            
            retries++;
            goto retry;
        }
        
        list_remove(&lthread_list, idx, 1);
        
        return luaL_exception_extended(L, LUA_THREAD_ERR_CANNOT_START, strerror(errno));
    }

    // Update thread status
    thread->status = LTHREAD_STATUS_RUNNING;
    
    // Store pthread id
    thread->thread = id;            

    // Return lthread id
    lua_pushinteger(L, idx);
    return 1;
}

// Create a new thread and run it
static int thread_start(lua_State* L) {
    return new_thread(L, 1);
}    

// Create a new thread in suspended mode
static int thread_create(lua_State* L) {
    return new_thread(L, 0);
}    

static int thread_sleep(lua_State* L) {
    int seconds;
    
    // Check argument (seconds)
    seconds = luaL_checkinteger(L, 1);
    
    sleep(seconds);
    
    return 0;
}

static int thread_sleepms(lua_State* L) {
    int milliseconds;
    
    // Check argument (seconds)
    milliseconds = luaL_checkinteger(L, 1);
    
    usleep(milliseconds * 1000);
    
    return 0;
}

static int thread_sleepus(lua_State* L) {
    int useconds;
    
    // Check argument (seconds)
    useconds = luaL_checkinteger(L, 1);
    
    usleep(useconds);
    
    return 0;
}

// Suspend all threads, or a specific thread
static int thread_suspend(lua_State* L) {
    return thread_suspend_pthreads(L, luaL_optinteger(L, 1, 0));    
}

// Resume all threads, or a specific thread
static int thread_resume(lua_State* L) {
    return thread_resume_pthreads(L, luaL_optinteger(L, 1, 0));    
}

// Stop all threads, or a specific thread
static int thread_stop(lua_State* L) {
    return thread_stop_pthreads(L, luaL_optinteger(L, 1, 0));    
}

static int thread_status(lua_State* L) {
    struct lthread *thread;
    int res;
    int thid;
    
    thid = luaL_checkinteger(L, 1);
    
    res = list_get(&lthread_list, thid, (void **)&thread);
    if (res) {
        lua_pushnil(L);
        return 1;
    }
    
    switch (thread->status) {
        case LTHREAD_STATUS_RUNNING:   lua_pushstring(L,"running"); break;
        case LTHREAD_STATUS_SUSPENDED: lua_pushstring(L,"suspended"); break;
    }

    return 1;
}

#include "modules.h"

extern LUA_REG_TYPE thread_error_map[];

static const LUA_REG_TYPE thread[] = {
    { LSTRKEY( "status" ),			LFUNCVAL( thread_status ) },
    { LSTRKEY( "create" ),			LFUNCVAL( thread_create ) },
    { LSTRKEY( "start" ),			LFUNCVAL( thread_start ) },
    { LSTRKEY( "suspend" ),			LFUNCVAL( thread_suspend ) },
    { LSTRKEY( "resume" ),			LFUNCVAL( thread_resume ) },
    { LSTRKEY( "stop" ),			LFUNCVAL( thread_stop ) },
    { LSTRKEY( "list" ),			LFUNCVAL( thread_list ) },
    { LSTRKEY( "sleep" ),			LFUNCVAL( thread_sleep ) },
    { LSTRKEY( "sleepms" ),			LFUNCVAL( thread_sleepms ) },
    { LSTRKEY( "sleepus" ),			LFUNCVAL( thread_sleepus ) },
    { LSTRKEY( "usleep" ),			LFUNCVAL( thread_sleepus ) },

	// Error definitions
	{LSTRKEY("error"),  			LROVAL( thread_error_map )},

	{ LNILKEY, LNILVAL }
};

int luaopen_thread(lua_State* L) {
	list_init(&lthread_list, 1);
	
#if !LUA_USE_ROTABLE
    luaL_newlib(L, thread);
    return 1;
#else
	return 0;
#endif
} 
 
MODULE_REGISTER_MAPPED(THREAD, thread, thread, luaopen_thread);
DRIVER_REGISTER(THREAD,thread,NULL,NULL,NULL);

#endif

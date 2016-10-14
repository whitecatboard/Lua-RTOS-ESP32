/*
 * Whitecat, pthread wrapper for whitecat
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

#if LUA_USE_THREAD

#include "lua.h"
#include "lapi.h"
#include "lauxlib.h"
#include "lgc.h"
#include "lmem.h"
#include "ldo.h"
#include "thread.h"

#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

#include "pthread.h"

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
            
        list_remove(&lthread_list, *thid);
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
    
    if (thid) {
        idx = thid;
    } else {
        idx = list_first(&lthread_list);
    }
    
    while (idx >= 0) {
        res = list_get(&lthread_list, idx, (void **)&thread);
        if (res) {
            return luaL_error(L, "can't suspend thread %d", idx);
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
    
    if (thid) {
        idx = thid;
    } else {
        idx = list_first(&lthread_list);
    }
    
    while (idx >= 0) {
        res = list_get(&lthread_list, idx, (void **)&thread);
        if (res) {
            return luaL_error(L, "can't resume thread %d", idx);
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
            return luaL_error(L, "can't stop thread %d", idx);
        }

        // If thread is created, suspend
        if (thread->thread) {
            _pthread_stop(thread->thread);            
            _pthread_free(thread->thread);

            luaL_unref(L, LUA_REGISTRYINDEX, thread->function_ref);
            luaL_unref(L, LUA_REGISTRYINDEX, thread->thread_ref);

            list_remove(&lthread_list, idx);
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
	lua_lock(L);
	luaC_fullgc(L, 1);
	lua_unlock(L);

	// Delay a number of ticks for take the iddle
	// task an opportunity for free allocated memory
	vTaskDelay(10);	
	
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
        printf("THID\tNAME\t\tSTATUS\tTIME\n");

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

            printf("%d\t%s\t\t%s\t%d\n", idx, "", status, 0);

            idx = list_next(&lthread_list, idx);
        }        
    }
    
    return 0;
}

static int new_thread(lua_State* L, int run) {
    struct lthread *thread;
    pthread_attr_t attr;
    int res, idx;
    pthread_t id;
    int retries;
    
    // Allocate space for lthread info
    thread = (struct lthread *)malloc(sizeof(struct lthread));
    if (!thread) {
        return luaL_error(L, "not enough memory");
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
        return luaL_error(L, "not enough memory");
    }

    // Create pthread
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, defaultThreadStack);

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
        
        list_remove(&lthread_list, idx);
        
        return luaL_error(L, "can't start pthread (%s)",strerror(errno));
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
 
LUA_OS_MODULE(THREAD, thread, thread);

#endif
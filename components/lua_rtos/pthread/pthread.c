/*
 * Lua RTOS, pthread implementation over FreeRTOS
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

#include "esp_attr.h"

#include "lua.h"
#include "thread.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/mutex.h>
#include <sys/queue.h>
#include <pthread/pthread.h>
#include "freertos/adds.h"

#include "lauxlib.h"

struct list key_list;
struct list mutex_list;
struct list thread_list;

struct mtx once_mtx;
struct mtx cond_mtx;

struct pthreadTaskArg {
    void *(*pthread_function) (void *);
    void *args;
    int id;
    int initial_state;
    int stack;
};
  
void pthreadTask(void *task_arguments);

void _pthread_init() {
    // Create mutexes
    mtx_init(&once_mtx, NULL, NULL, 0);
    mtx_init(&cond_mtx, NULL, NULL, 0);
    
    // Init lists
    list_init(&thread_list, 1);
    list_init(&mutex_list, 1);
    list_init(&key_list, 1);
}

int _pthread_create(pthread_t *id, int priority, int stacksize, int cpu, int initial_state,
                    void *(*start_routine)(void *), void *args
) {
    xTaskHandle xCreatedTask;              // Related task
    struct pthreadTaskArg *taskArgs;
    BaseType_t res;
    struct pthread *thread;
    struct pthread *parent_thread;
    int current_thread, i;
    
    // Set pthread arguments for creation
    taskArgs = (struct pthreadTaskArg *)malloc(sizeof(struct pthreadTaskArg));
    if (!taskArgs) {
        errno = EAGAIN;
        return EAGAIN;
    }
                
    taskArgs->pthread_function = start_routine;
    taskArgs->args = args;
    taskArgs->initial_state = initial_state;
    
    // Create thread structure
    thread = (struct pthread *)malloc(sizeof(struct pthread));
    if (!thread) {
        free(taskArgs);
        errno = EAGAIN;
        return EAGAIN;
    }
    
    for(i=0; i < PTHREAD_NSIG; i++) {
        thread->signals[i] = SIG_DFL;
    }

    current_thread = pthread_self();
    if (current_thread > 0) {
        // Get parent thread
        res = list_get(&thread_list, current_thread, (void **)&parent_thread);
        if (res) {
            free(taskArgs);
            errno = EAGAIN;
            return EAGAIN;
        }
        
        // Copy signals to new thread
        bcopy(parent_thread->signals, thread->signals, sizeof(sig_t) * PTHREAD_NSIG);
    }
    
    list_init(&thread->join_list, 1);
    list_init(&thread->clean_list, 1);
    
    mtx_init(&thread->init_mtx, NULL, NULL, 0);

    // Add to the thread list
    res = list_add(&thread_list, thread, id);
    if (res) {
        free(taskArgs);
        free(thread);
        errno = EAGAIN;
        return EAGAIN;
    }

    taskArgs->id = *id;
    taskArgs->stack = stacksize;
    thread->thread = *id;
    
    // This is the parent thread. After the creation of the new task related to new thread
    // we need to wait for the initialization of critical information that is provided by
    // new thread:
    //
    // * Allocate Lua RTOS specific TCB parts, using local storage pointer assigned to pthreads
    // * CPU core id when thread is running
    // * The thread id, stored in Lua RTOS specific TCB parts
    // * The Lua state, stored in Lua RTOS specific TCB parts
    //
    // This is done by pthreadTask function, who releases the lock when this information is set
    mtx_lock(&thread->init_mtx);

    // Create related task
    if (cpu == tskNO_AFFINITY) {
        res = xTaskCreate(pthreadTask, "lthread", stacksize, taskArgs, priority, &xCreatedTask);
    } else {
        res = xTaskCreatePinnedToCore(&pthreadTask, "lthread", stacksize, taskArgs,priority, &xCreatedTask, cpu);
    }

    if(res != pdPASS) {
        // Remove from thread list
        list_remove(&thread_list,*id, 1);
        free(taskArgs);
        free(thread);

        if (res == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) {
            errno = ENOMEM;
            return ENOMEM;
        } else {
            errno = EAGAIN;
            return EAGAIN;
        }
    }

    // Wait for the initialization of Lua RTOS specific TCB parts
    mtx_lock(&thread->init_mtx);
    
    thread->task = xCreatedTask;

    return 0;
}

int _pthread_join(pthread_t id) {
    struct pthread_join *join;
    struct pthread *thread;
    int idx;
    int res;
    char c;

    // Get thread
    res = list_get(&thread_list, id, (void **)&thread);
    if (res) {
        errno = res;
        return res;
    }
    
    // Create join structure
    join = (struct pthread_join *)malloc(sizeof(struct pthread_join));
    if (!join) {
        errno = ESRCH;
        return ESRCH;
    }
    
    // Create a queue
    join->queue = xQueueCreate(1, sizeof(char));
    if (join->queue == 0) {
        free(join);
        errno = ESRCH;
        return ESRCH;            
    }

    // Add join to the join list
    res = list_add(&thread->join_list, join, &idx); 
    if (res) {
        vQueueDelete(join->queue);
        free(join);
        errno = ESRCH;
        return ESRCH;
    }

    // Wait
    xQueueReceive(join->queue,&c,portMAX_DELAY);
        
    return 0;
} 

int _pthread_free(pthread_t id) {
    struct pthread *thread;
    int res;
    
    _pthread_mutex_free();
    
    // Get thread
    res = list_get(&thread_list, id, (void **)&thread);
    if (res) {
        errno = res;
        return res;
    }

    // Free join list
    list_destroy(&thread->join_list, 1);

    // Free clean list
    list_destroy(&thread->clean_list, 1);
    
    mtx_destroy(&thread->init_mtx);    
    
    // Remove thread
    list_remove(&thread_list, id, 1);
    
    return 0;
}

sig_t _pthread_signal(int s, sig_t h) {
    struct pthread *thread; // Current thread
    sig_t prev_h;           // Previous handler
    
    if (s > PTHREAD_NSIG) {
        errno = EINVAL;
        return SIG_ERR;
    }
    
    // Get thread
    list_get(&thread_list, pthread_self(), (void **)&thread);
    
    if (thread) {
        // Add handler
        prev_h = thread->signals[s];
        thread->signals[s] = h;

        return prev_h;
    }

    return NULL;
}

void IRAM_ATTR _pthread_queue_signal(int s) {
    struct pthread *thread; // Current thread
    int index;
    
    index = list_first(&thread_list);
    while (index >= 0) {
        list_get(&thread_list, index, (void **)&thread);
        
        if (thread->thread == 1) {
            if ((thread->signals[s] != SIG_DFL) && (thread->signals[s] != SIG_IGN)) {
				#if LUA_USE_SAFE_SIGNAL
            	uxSetSignaled(thread->task, s);
				#else
            	thread->signals[s](s);
				#endif
            }         
        }
        
        index = list_next(&thread_list, index);
    }
}

#if LUA_USE_SAFE_SIGNAL
void _pthread_process_signal(void) {
    struct pthread *thread; // Current thread
    uint32_t s;
    
    list_get(&thread_list, pthread_self(), (void **)&thread);
    
    for(s=0;s < 32;s++) {
    	if (uxGetSignaled(thread->task) & (1 << s)) {
	        if ((thread->signals[s] != SIG_DFL) && (thread->signals[s] != SIG_IGN)) {
    			thread->signals[s](s);
	        }
	        uxSetSignaled(thread->task, 0);
    	}
    }
}
#endif

int IRAM_ATTR _pthread_has_signal(int s) {
    struct pthread *thread; // Current thread
    int index;
    
    index = list_first(&thread_list);
    while (index >= 0) {
        list_get(&thread_list, index, (void **)&thread);
        
        if (thread->thread == 1) {
            if ((thread->signals[s] != SIG_DFL) && (thread->signals[s] != SIG_IGN)) {
                return 1;
            }
        }
        
        index = list_next(&thread_list, index);
    }    

    return 0;
}

int _pthread_stop(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get thread
    res = list_get(&thread_list, id, (void **)&thread);
    if (res) {
        errno = res;
        return res;
    }

    // Stop
    vTaskDelete(thread->task);
    
    return 0;
}

int _pthread_core(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get thread
    res = list_get(&thread_list, id, (void **)&thread);
    if (res) {
        errno = res;
        return res;
    }

    return (int)uxGetCoreID(thread->task);
}

int _pthread_stack(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get thread
    res = list_get(&thread_list, id, (void **)&thread);
    if (res) {
        errno = res;
        return res;
    }

    return (int)uxGetStack(thread->task);
}

int _pthread_stack_free(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get thread
    res = list_get(&thread_list, id, (void **)&thread);
    if (res) {
        errno = res;
        return res;
    }

    return (int)uxTaskGetStackHighWaterMark(thread->task);
}

int _pthread_suspend(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get thread
    res = list_get(&thread_list, id, (void **)&thread);
    if (res) {
        errno = res;
        return res;
    }
    
    // Stop
    vTaskSuspend(thread->task);
    
    return 0;
}

int _pthread_resume(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get thread
    res = list_get(&thread_list, id, (void **)&thread);
    if (res) {
        errno = res;
        return res;
    }
        
    // Stop
    vTaskResume(thread->task);

    return 0;
}

// This is the callback function for free Lua RTOS specific TCB parts
static void pthreadLocaleStoragePointerCallback(int index, void* data) {
	if (index == THREAD_LOCAL_STORAGE_POINTER_ID) {
		free(data);
	}
}

void pthreadTask(void *taskArgs) {
    struct pthreadTaskArg *args; 		  // Task arguments
    struct pthread_join *join;   		  // Current join
    struct pthread_clean *clean; 	  	  // Current clean
    struct pthread *thread;      		  // Current thread
	lua_rtos_tcb_t *lua_rtos_tcb;         // Lua RTOS specific TCB parts
	
    char c = '1';
    int index;

    // This is the new thread
    args = (struct pthreadTaskArg *)taskArgs;
	
    // Get thread
    list_get(&thread_list, args->id, (void **)&thread);

	// Allocate and init Lua RTOS specific TCB parts, and store into a FreeRTOS
	// local storage pointer
	lua_rtos_tcb = (lua_rtos_tcb_t *)calloc(1, sizeof(lua_rtos_tcb_t));
	if (!lua_rtos_tcb) {
		abort();
	}
		
	vTaskSetThreadLocalStoragePointerAndDelCallback(NULL, THREAD_LOCAL_STORAGE_POINTER_ID, (void *)lua_rtos_tcb, pthreadLocaleStoragePointerCallback);

	// Store CPU core id when thread is running
	uxSetCoreID(xPortGetCoreID());

	// Store stack size
	uxSetStack(args->stack);

    // Set thread id
    uxSetThreadId(args->id);

#if LUA_USE_THREAD
	if (args->args) {
		// Set Lua context into TCB
		uxSetLuaState(((struct lthread *)args->args)->L);
	}
#endif
		
	// Lua RTOS specific TCB parts are set, parent thread can continue
    mtx_unlock(&thread->init_mtx);
    
    if (args->initial_state == PTHREAD_INITIAL_STATE_SUSPEND) {
        vTaskSuspend(NULL);
    }
    
    // Call start function
    int *status = args->pthread_function(args->args);
#if LUA_USE_THREAD
    if (status) {
        if (*status != LUA_OK) {
            struct lthread *thread;
            thread = (struct lthread *)args->args;

            const char *msg = lua_tostring(thread->L, -1);
            lua_writestringerror("%s\n", msg);
            lua_pop(thread->L, 1);
        }

        free(status);
    }
#endif
    // Inform from thread end to joined threads
    index = list_first(&thread->join_list);
    while (index >= 0) {
        list_get(&thread->join_list, index, (void **)&join);
                
        xQueueSend(join->queue,&c, portMAX_DELAY);
        
        index = list_next(&thread->join_list, index);
    }

    // Execute clean list
    index = list_first(&thread->clean_list);
    while (index >= 0) {
        list_get(&thread->clean_list, index, (void **)&clean);
         
        if (clean->clean) {
            (*clean->clean)(clean->args);
            free(clean->args);
        }
        
        index = list_next(&thread->clean_list, index);
    }
    
    // Free thread structures
    _pthread_free(args->id);

    // Free args
	free(taskArgs);

    // End related task
    vTaskDelete(NULL);
}

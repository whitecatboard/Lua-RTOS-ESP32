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
 * Lua RTOS pthread implementation for FreeRTOS
 *
 */

#include "luartos.h"

#include "esp_err.h"
#include "esp_attr.h"

#include "thread.h"
#include "_pthread.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/mutex.h>
#include <sys/queue.h>

// A mutex to sync with critical parts
static struct mtx thread_mtx;

// List of active threads. An active thread is one that is created
// and has not yet terminated.
static struct list active_threads;

// List of inactive threads. An inactive thread is one that has yet
// terminated, but that was not joined. We must to store this threads
// in a list to allow joins after the thread termination.
static struct list inactive_threads;

struct list key_list;

static uint8_t inited = 0;

// Arguments for the thread task
struct pthreadTaskArg {
    struct pthread *thread;                // Thread data
    void *(*pthread_function)(void *); // Thread start routine
    void *args;                         // Thread start routine arguments
    xTaskHandle parent_task;            // Handle of parent task
};

void pthreadTask(void *task_arguments);

void _pthread_lock() {
    mtx_lock(&thread_mtx);
}

void _pthread_unlock() {
    mtx_unlock(&thread_mtx);
}

void _pthread_init() {
    if (!inited) {
        // Create mutexes
        mtx_init(&thread_mtx, NULL, NULL, 0);

        // Init lists
        lstinit(&active_threads, 1, LIST_NOT_INDEXED);
        lstinit(&inactive_threads, 1, LIST_NOT_INDEXED);

        lstinit(&key_list, 1, LIST_DEFAULT);

        inited = 1;
    }
}

int _pthread_create(pthread_t *thread, const pthread_attr_t *attr,
        void *(*start_routine)(void *), void *args) {
    BaseType_t res;

    // Get the creation attributes
    pthread_attr_t cattr;
    if (attr) {
        // Creation attributes passed as function arguments, copy to thread data
        memcpy(&cattr, attr, sizeof(pthread_attr_t));
    } else {
        // Apply default creation attributes
        pthread_attr_init(&cattr);
    }

    // Create and populate the thread data
    struct pthread *threadd;

    threadd = (struct pthread *) calloc(1, sizeof(struct pthread));
    if (!threadd) {
        return EAGAIN;
    }

    // Thread is active
    threadd->active = 1;

    // Copy creation attributes
    memcpy(&threadd->attr, &cattr, sizeof(pthread_attr_t));

    // No task joined
    threadd->joined_task = NULL;

    // No result
    threadd->res = NULL;

    // Initialize signal handlers
    pthread_t self;

    if ((self = pthread_self())) {
        // Copy signal handlers from current thread
        bcopy(((struct pthread *) self)->signals, threadd->signals,
                sizeof(sig_t) * PTHREAD_NSIG);
    } else {
        // Set default signal handlers
        int i;

        for (i = 0; i < PTHREAD_NSIG; i++) {
            threadd->signals[i] = SIG_DFL;
        }
    }

    // Init clean list
    lstinit(&threadd->clean_list, 1, LIST_DEFAULT);

    // Add thread to the thread list
    res = lstadd(&active_threads, (void *) threadd, NULL);
    if (res) {
        free(threadd);
        return EAGAIN;
    }

    *thread = (pthread_t) threadd;

    // Create and populate the arguments for the thread task
    struct pthreadTaskArg *taskArgs;

    taskArgs = (struct pthreadTaskArg *) calloc(1,
            sizeof(struct pthreadTaskArg));
    if (!taskArgs) {
        lstremove(&active_threads, (int) threadd, 1);
        return EAGAIN;
    }

    taskArgs->thread = threadd;
    taskArgs->pthread_function = start_routine;
    taskArgs->args = args;
    taskArgs->parent_task = xTaskGetCurrentTaskHandle();

    // This is the parent thread. Now we need to wait for the initialization of critical information
    // that is provided by the pthreadTask:
    //
    // * Allocate Lua RTOS specific TCB parts, using local storage pointer assigned to pthreads
    // * CPU core id when thread is running
    // * The thread id, stored in Lua RTOS specific TCB parts
    // * The Lua state, stored in Lua RTOS specific TCB parts
    //

    // Create related task
    int cpu = 0;

    if (cattr.schedparam.affinityset != CPU_INITIALIZER) {
        if (CPU_ISSET(0, &cattr.schedparam.affinityset)) {
            cpu = 0;
        } else if (CPU_ISSET(1, &cattr.schedparam.affinityset)) {
            cpu = 1;
        }
    } else {
        cpu = tskNO_AFFINITY;
    }

    xTaskHandle xCreatedTask; // Related task

    if (cpu == tskNO_AFFINITY) {
        res = xTaskCreate(pthreadTask, "thread", cattr.stacksize, taskArgs,
                cattr.schedparam.sched_priority, &xCreatedTask);
    } else {
        res = xTaskCreatePinnedToCore(pthreadTask, "thread", cattr.stacksize,
                taskArgs, cattr.schedparam.sched_priority, &xCreatedTask, cpu);
    }

    if (res != pdPASS) {
        // Remove from thread list
        lstremove(&active_threads, *thread, 1);
        free(taskArgs);

        return EAGAIN;
    }

    // Wait for the initialization of Lua RTOS specific TCB parts
    xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

    threadd->task = xCreatedTask;

    return 0;
}

int _pthread_join(pthread_t id, void **value_ptr) {
    _pthread_lock();

    // Get thread
    struct pthread *thread;

    if (lstget(&active_threads, id, (void **) &thread) != 0) {
        // Thread not found in active threads, may be is inactive
        if (lstget(&inactive_threads, id, (void **) &thread) != 0) {
            // Thread not found
            _pthread_unlock();
            return ESRCH;
        }
    }

    // Check that thread is joinable
    if (thread->attr.detachstate == PTHREAD_CREATE_DETACHED) {
        _pthread_unlock();

        return EINVAL;
    }

    if (thread->active) {
        if (thread->joined_task != NULL) {
            _pthread_unlock();

            // Another thread is already waiting to join with this thread
            return EINVAL;
        }

        // Join the current task to the thread
        thread->joined_task = xTaskGetCurrentTaskHandle();
    }

    if (thread->active) {
        _pthread_unlock();

        // Wait until the thread ends
        uint32_t ret;

        xTaskNotifyWait(0, 0, &ret, portMAX_DELAY);

        if (value_ptr) {
            *value_ptr = (void *) ret;
        }
    } else {
        if (value_ptr) {
            *value_ptr = (void *) thread->res;
        }

        _pthread_free((pthread_t) thread, 1);

        _pthread_unlock();
    }

    return 0;
}

int _pthread_detach(pthread_t id) {
    _pthread_lock();

    // Get thread
    struct pthread *thread;

    if (lstget(&active_threads, id, (void **) &thread) != 0) {
        // Thread not found in active threads, may be is inactive
        if (lstget(&inactive_threads, id, (void **) &thread) != 0) {
            // Thread not found
            _pthread_unlock();
            return ESRCH;
        }
    }

    if (thread->attr.detachstate == PTHREAD_CREATE_DETACHED) {
        return EINVAL;
    }

    thread->attr.detachstate = PTHREAD_CREATE_DETACHED;

    lstremove(&inactive_threads, id, 1);

    _pthread_unlock();

    return 0;
}

void _pthread_cleanup_push(void (*routine)(void *), void *arg) {
    struct pthread *thread;
    struct pthread_clean *clean;

    _pthread_lock();

    // Get current thread
    if (lstget(&active_threads, pthread_self(), (void **) &thread) == 0) {
        // Create the clean structure
        clean = (struct pthread_clean *) malloc(sizeof(struct pthread_clean));
        if (!clean) {
            _pthread_unlock();
            return;
        }

        clean->clean = routine;
        clean->args = arg;

        // Add to clean list
        lstadd(&thread->clean_list, clean, NULL);
    }

    _pthread_unlock();
}

void _pthread_cleanup_pop(int execute) {
    struct pthread *thread;
    struct pthread_clean *clean;

    _pthread_lock();

    // Get current thread
    if (lstget(&active_threads, pthread_self(), (void **) &thread) == 0) {
        // Get last element in clean list, so we must pop handlers in reverse order
        int index;

        if ((index = lstlast(&thread->clean_list)) >= 0) {
            if (lstget(&thread->clean_list, index, (void **) &clean) == 0) {
                // Execute handler
                if (clean->clean && execute) {
                    clean->clean(clean->args);
                }

                // Remove handler from list
                lstremove(&thread->clean_list, index, 1);
            }
        }
    }

    _pthread_unlock();
}

void _pthread_cleanup() {
    struct pthread *thread;
    struct pthread_clean *clean;

    _pthread_lock();

    // Get current thread
    if (lstget(&active_threads, pthread_self(), (void **) &thread) == 0) {
        // Get all elements in clean list, in reverse order
        int index;

        while ((index = lstlast(&thread->clean_list)) >= 0) {
            if (lstget(&thread->clean_list, index, (void **) &clean) == 0) {
                // Execute handler
                if (clean->clean) {
                    clean->clean(clean->args);
                }

                // Remove handler from list
                lstremove(&thread->clean_list, index, 1);
            }
        }
    }

    _pthread_unlock();
}

int _pthread_free(pthread_t id, int destroy) {
    // Get thread
    struct pthread *thread = (struct pthread *) id;

    // Destroy clean list
    lstdestroy(&thread->clean_list, 1);

    // Remove thread
    lstremove(&active_threads, id, destroy);
    lstremove(&inactive_threads, id, destroy);

    return 0;
}

sig_t _pthread_signal(int s, sig_t h) {
    struct pthread *thread; // Current thread
    sig_t prev_h;           // Previous handler

    if (s > PTHREAD_NSIG) {
        return NULL;
    }

    if (lstget(&active_threads, pthread_self(), (void **) &thread) == 0) {
        // Add handler
        prev_h = thread->signals[s];
        thread->signals[s] = h;

        return prev_h;
    }

    return NULL;
}

void _pthread_exec_signal(int dst, int s) {
    if (s > PTHREAD_NSIG) {
        return;
    }

    // Get destination thread
    struct pthread *thread;

    if (lstget(&active_threads, dst, (void **) &thread) == 0) {
        // If destination thread has a handler for the signal, execute it
        if ((thread->signals[s] != SIG_DFL)
                && (thread->signals[s] != SIG_IGN)) {
            if (thread->is_delayed && ((s == (SIGINT) || (s == SIGABRT)))) {
                xTaskAbortDelay(thread->task);
            }

            thread->signals[s](s);
        }
    }
}

int IRAM_ATTR _pthread_has_signal(int dst, int s) {
    if (s > PTHREAD_NSIG) {
        return 0;
    }

    // Get destination thread
    struct pthread *thread;

    if (lstget(&active_threads, dst, (void **) &thread) == 0) {
        return ((thread->signals[s] != SIG_DFL)
                && (thread->signals[s] != SIG_IGN));
    }

    return 0;
}

int _pthread_sleep(uint32_t msecs) {
    struct pthread *thread;
    int res;

    // Get the current thread
    res = lstget(&active_threads, pthread_self(), (void **) &thread);
    if (res) {
        // Its not a thread, simply delay task
        vTaskDelay(msecs / portTICK_PERIOD_MS);
        return 0;
    }

    // Is a thread. Mark it as delayed.
    thread->is_delayed = 1;
    thread->delay_interrupted = 0;

    vTaskDelay(msecs / portTICK_PERIOD_MS);

    thread->is_delayed = 0;

    if (thread->delay_interrupted) {
        thread->delay_interrupted = 0;
        return -1;
    } else {
        return 0;
    }
}

int _pthread_stop(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get thread
    res = lstget(&active_threads, id, (void **) &thread);
    if (res) {
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
    res = lstget(&active_threads, id, (void **) &thread);
    if (res) {
        return res;
    }

    return (int) ucGetCoreID(thread->task);
}

int _pthread_stack(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get thread
    res = lstget(&active_threads, id, (void **) &thread);
    if (res) {
        return res;
    }

    return (int) uxGetStack(thread->task);
}

int _pthread_stack_free(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get thread
    res = lstget(&active_threads, id, (void **) &thread);
    if (res) {
        return res;
    }

    return (int) uxTaskGetStackHighWaterMark(thread->task);
}

int _pthread_suspend(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get thread
    res = lstget(&active_threads, id, (void **) &thread);
    if (res) {
        return res;
    }

    UBaseType_t priority = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, 22);

    // Suspend
    uxSetThreadStatus(thread->task, StatusSuspended);
    vTaskSuspend(thread->task);

    vTaskPrioritySet(NULL, priority);

    return 0;
}

int _pthread_resume(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get thread
    res = lstget(&active_threads, id, (void **) &thread);
    if (res) {
        return res;
    }

    // Resume
    uxSetThreadStatus(thread->task, StatusRunning);
    vTaskResume(thread->task);

    return 0;
}

struct pthread *_pthread_get(pthread_t id) {
    struct pthread *thread;
    int res;

    // Get the thread
    res = lstget(&active_threads, id, (void **) &thread);
    if (res) {
        return NULL;
    }

    return thread;
}

// This is the callback function for free Lua RTOS specific TCB parts
static void pthreadLocaleStoragePointerCallback(int index, void* data) {
    if (index == THREAD_LOCAL_STORAGE_POINTER_ID) {
        free(data);
    }
}

void pthreadTask(void *taskArgs) {
    struct pthreadTaskArg *args;           // Task arguments
    struct pthread *thread;                // Current thread
    lua_rtos_tcb_t *lua_rtos_tcb;         // Lua RTOS specific TCB parts

    // This is the new thread
    args = (struct pthreadTaskArg *) taskArgs;

    // Get thread
    thread = (struct pthread *) args->thread;

    // Allocate and initialize Lua RTOS specific TCB parts, and store it into a FreeRTOS
    // local storage pointer
    lua_rtos_tcb = (lua_rtos_tcb_t *) calloc(1, sizeof(lua_rtos_tcb_t));
    assert(lua_rtos_tcb != NULL);

    lua_rtos_tcb->status = StatusRunning;

    vTaskSetThreadLocalStoragePointerAndDelCallback(NULL,
    THREAD_LOCAL_STORAGE_POINTER_ID, (void *) lua_rtos_tcb,
            pthreadLocaleStoragePointerCallback);

    // Set thread id
    uxSetThreadId((UBaseType_t) args->thread);

    // Call additional thread init function
    if (args->thread->attr.init_func) {
        args->thread->attr.init_func(args->args);
    }

    // Lua RTOS specific TCB parts are set, parent thread can continue
    xTaskNotify(args->parent_task, 0, eNoAction);

    if (args->thread->attr.schedparam.initial_state
            == PTHREAD_INITIAL_STATE_SUSPEND) {
        vTaskSuspend(NULL);
    }

    // Call start function
    void *ret = args->pthread_function(args->args);

    _pthread_lock();

    if (args->thread->attr.detachstate == PTHREAD_CREATE_JOINABLE) {
        if (thread->joined_task != NULL) {
            // Notify to joined thread
            xTaskNotify(thread->joined_task, (uint32_t) ret,
                    eSetValueWithOverwrite);

            thread->joined_task = NULL;

            _pthread_free((pthread_t) thread, 1);
        } else {
            _pthread_free((pthread_t) thread, 0);

            // Put the thread into the inactive threads, because join can occur later, and
            // the thread's result must be stored
            thread->active = 0;
            thread->res = ret;

            lstadd(&inactive_threads, (void *) thread, NULL);
        }
    } else {
        _pthread_free((pthread_t) thread, 1);
    }

    // Free args
    free(taskArgs);

    _pthread_unlock();

    // End related task
    vTaskDelete(NULL);
}

int pthread_cancel(pthread_t thread) {
    return 0;
}

esp_err_t esp_pthread_init(void) {
    _pthread_init();

    return ESP_OK;
}

int pthread_setname_np(pthread_t id, const char *name) {
    struct pthread *thread;
    int res;

    // Sanity checks
    if (strlen(name) > configMAX_TASK_NAME_LEN - 1) {
        return ERANGE;
    }

    // Get the thread
    res = lstget(&active_threads, id, (void **) &thread);
    if (res) {
        return EINVAL;
    }

    // Get the TCB task for this thread
    tskTCB_t *task = (tskTCB_t *) (thread->task);

    // Copy the name into the TCB
    strncpy(task->pcTaskName, name, configMAX_TASK_NAME_LEN - 1);

    return 0;
}

int pthread_getname_np(pthread_t id, char *name, size_t len) {
    struct pthread *thread;
    int res;

    // Get the thread
    res = lstget(&active_threads, id, (void **) &thread);
    if (res) {
        return EINVAL;
    }

    // Get the TCB task for this thread
    tskTCB_t *task = (tskTCB_t *) (thread->task);

    // Sanity checks
    if (strlen(task->pcTaskName) < len - 1) {
        return ERANGE;
    }

    // Copy the name from the TCB
    strncpy(name, task->pcTaskName, configMAX_TASK_NAME_LEN - 1);

    return 0;
}

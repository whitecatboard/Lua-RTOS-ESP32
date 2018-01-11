/*
 * Lua RTOS, FreeRTOS adds needed for Lua RTOS
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

#include "esp_attr.h"

#include "lua.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/adds.h"

#include <stdint.h>
#include <string.h>

// Global state
static lua_State *gL = NULL;

static int compare(const void *a, const void *b) {
	if (((task_info_t *)a)->task_type < ((task_info_t *)b)->task_type) {
		return 1;
	} else if (((task_info_t*)a)->task_type > ((task_info_t *)b)->task_type) {
		return -1;
	} else {
		if (((task_info_t *)a)->core < ((task_info_t *)b)->core) {
			return -1;
		} else if (((task_info_t *)a)->core > ((task_info_t *)b)->core) {
			return 1;
		} else {
			return strcmp(((task_info_t *)a)->name, ((task_info_t *)b)->name);
		}
	}
}

void uxSetThreadId(UBaseType_t id) {
	lua_rtos_tcb_t *lua_rtos_tcb;
	
	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(xTaskGetCurrentTaskHandle(), THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Store thread id into Lua RTOS specific TCB parts
		lua_rtos_tcb->threadid = id;		
	}
}

UBaseType_t IRAM_ATTR uxGetThreadId() {
	lua_rtos_tcb_t *lua_rtos_tcb;
	int threadid = 0;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(xTaskGetCurrentTaskHandle(), THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Get current thread od from Lua RTOS specific TCB parts
		threadid = lua_rtos_tcb->threadid;
	}

	return threadid;
}

void uxSetThreadStatus(TaskHandle_t h, pthread_status_t status) {
	lua_rtos_tcb_t *lua_rtos_tcb;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(xTaskGetCurrentTaskHandle(), THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Store thread id into Lua RTOS specific TCB parts
		lua_rtos_tcb->status = status;
	}
}

void uxSetLThread(lthread_t *lthread) {
	lua_rtos_tcb_t *lua_rtos_tcb;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(xTaskGetCurrentTaskHandle(), THREAD_LOCAL_STORAGE_POINTER_ID))) {
		lua_rtos_tcb->lthread = lthread;
	}
}

void uxSetLuaState(lua_State* L) {
	lua_rtos_tcb_t *lua_rtos_tcb;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(xTaskGetCurrentTaskHandle(), THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Store current lua state into Lua RTOS specific TCB parts
		if (!lua_rtos_tcb->lthread) {
			lua_rtos_tcb->lthread = calloc(1, sizeof(lthread_t));
			assert(lua_rtos_tcb->lthread);
		}
		lua_rtos_tcb->lthread->L = L;
	}

	// If this is the first thread (Lua thread) store
	// state in global state
	if ((uxGetThreadId() == 1) && (gL == NULL)) {
		gL = L;
	}
}

lua_State* pvGetLuaState() {
	lua_rtos_tcb_t *lua_rtos_tcb;
	lua_State *L = NULL;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_POINTER_ID))) {

		// Get current lua state from Lua RTOS specific TCB parts
		if (lua_rtos_tcb->lthread) {
			L = lua_rtos_tcb->lthread->L;
		}
	}

	if (L == NULL) {
		L = gL;
	}

	return L;
}

lthread_t *pvGetLThread() {
	lua_rtos_tcb_t *lua_rtos_tcb;
	lthread_t *lthread = NULL;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		lthread = lua_rtos_tcb->lthread;
	}

	return lthread;
}

uint32_t uxGetSignaled(TaskHandle_t h) {
	lua_rtos_tcb_t *lua_rtos_tcb;
	int signaled = 0;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(h, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Get current signeled mask from Lua RTOS specific TCB parts
		signaled = lua_rtos_tcb->signaled;
	}

	return signaled;
}

void IRAM_ATTR uxSetSignaled(TaskHandle_t h, int s) {
	lua_rtos_tcb_t *lua_rtos_tcb;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(h, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Store current signaled mask into Lua RTOS specific TCB parts
		lua_rtos_tcb->signaled = s;		
	}
}

uint8_t ucGetCoreID(TaskHandle_t h) {
	tskTCB_t *task = (tskTCB_t *)h;

	return task->xCoreID;
}

int uxGetStack(TaskHandle_t h) {
	tskTCB_t *task = (tskTCB_t *)h;

	return task->pxEndOfStack - task->pxStack + 4;
}

task_info_t *GetTaskInfo() {
	tskTCB_t *ctask;
	task_info_t *info;
	uint8_t task_type;
	lua_rtos_tcb_t *lua_rtos_tcb;
	TaskStatus_t *status_array;
	UBaseType_t task_num = 0;
	UBaseType_t start_task_num = 0;
	uint32_t total_runtime = 0;

	//Allocate status_array
	start_task_num = uxTaskGetNumberOfTasks();

	status_array = (TaskStatus_t *)calloc(start_task_num, sizeof(TaskStatus_t));
	if (!status_array) {
		return NULL;
	}

	task_num = uxTaskGetSystemState(status_array, (start_task_num), &total_runtime);
	// For percentage calculations.
	total_runtime /= 100UL;

	info = (task_info_t *)calloc(task_num + 1, sizeof(task_info_t));
	if (!info) {
		free(status_array);
		return NULL;
	}

	for(int i = 0; i <task_num; i++){
		// Get the task TCB
		ctask = (tskTCB_t *)status_array[i].xHandle;

		// Get the task type
		// 0: freertos task
		// 1: pthread task
		// 2: lua thread task
		task_type = 0;

		info[i].thid = 0;
		info[i].status = 0;

		// Get Lua RTOS specific TCB parts for current task
		if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(status_array[i].xHandle, THREAD_LOCAL_STORAGE_POINTER_ID))) {
			// Task has Lua RTOS specific TCB parts
			if (lua_rtos_tcb->lthread) {
				// Lua thread
				task_type = 2;
			} else {
				// pthread
				task_type = 1;
			}

			info[i].thid = lua_rtos_tcb->threadid;
			info[i].lthread = lua_rtos_tcb->lthread;
			info[i].status = lua_rtos_tcb->status;
		}

		// Populate info item
		info[i].prio = status_array[i].uxCurrentPriority;
		info[i].task_type = task_type;
		info[i].core = ctask->xCoreID;

		// Some system tasks shows 255!!
		if (info[i].core > 1) {
			info[i].core = 0;
		}

		info[i].free_stack = uxTaskGetStackHighWaterMark(status_array[i].xHandle);
		info[i].stack_size = ctask->pxEndOfStack - ctask->pxStack + 4;
		memcpy(info[i].name, status_array[i].pcTaskName, configMAX_TASK_NAME_LEN);

		info[i].cpu_usage = 0;
#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
		if( total_runtime > 0 ) {
			// only gives valid values if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS is defined
			info[i].cpu_usage = status_array[i].ulRunTimeCounter / total_runtime;
		}
#endif

	}

	free(status_array);

	qsort (info, task_num, sizeof (task_info_t), compare);

	return info;
}

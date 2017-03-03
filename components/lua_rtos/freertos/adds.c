/*
 * Lua RTOS, FreeRTOS adds needed for Lua RTOS
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/adds.h"

#include <stdint.h>

#if 0
// Define a spinklock for protect critical regions in Lua RTOS
static portMUX_TYPE luartos_spinlock = portMUX_INITIALIZER_UNLOCKED;

// This is for determine if we are in an interrupt
extern unsigned port_interruptNesting[portNUM_PROCESSORS];

void enter_critical_section() {
	 if (port_interruptNesting[xPortGetCoreID()] != 0) {
		 portENTER_CRITICAL_ISR(&luartos_spinlock);
	 } else {
		 portENTER_CRITICAL(&luartos_spinlock);
	 }
}

void exit_critical_section() {
	if (port_interruptNesting[xPortGetCoreID()] != 0) {
		 portEXIT_CRITICAL_ISR(&luartos_spinlock);
	 } else {
		 portEXIT_CRITICAL(&luartos_spinlock);
	 }
}
#endif

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

void uxSetLuaState(lua_State* L) {
	lua_rtos_tcb_t *lua_rtos_tcb;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(xTaskGetCurrentTaskHandle(), THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Store current lua state into Lua RTOS specific TCB parts
		lua_rtos_tcb->L = L;		
	}
}

lua_State* pvGetLuaState() {
	lua_rtos_tcb_t *lua_rtos_tcb;
	lua_State *L = NULL;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_POINTER_ID))) {

		// Get current lua state from Lua RTOS specific TCB parts
		L = lua_rtos_tcb->L;
	}

	return L;
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

uint8_t uxGetCoreID(TaskHandle_t h) {
	lua_rtos_tcb_t *lua_rtos_tcb;
	int coreid = 0;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(h, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Get current signeled mask from Lua RTOS specific TCB parts
		coreid = lua_rtos_tcb->coreid;
	}

	return coreid;
}

int uxGetStack(TaskHandle_t h) {
	lua_rtos_tcb_t *lua_rtos_tcb;
	int stack = 0;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(h, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Get stack size from Lua RTOS specific TCB parts
		stack = lua_rtos_tcb->stack;
	}

	return stack;
}

void uxSetCoreID(int core) {
	lua_rtos_tcb_t *lua_rtos_tcb;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Store current signaled mask into Lua RTOS specific TCB parts
		lua_rtos_tcb->coreid = core;
	}
}

void uxSetStack(int stack) {
	lua_rtos_tcb_t *lua_rtos_tcb;

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Store stack size into Lua RTOS specific TCB parts
		lua_rtos_tcb->stack = stack;
	}
}

/*
 * Lua RTOS, FreeRTOS adds needed for Lua RTOS
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

#include "luartos.h"

#include "lua.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/adds.h"

#include <stdint.h>

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

void uxSetThreadId(UBaseType_t id) {
	struct lua_rtos_tcb *lua_rtos_tcb;
	
	enter_critical_section();

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Store thread id into Lua RTOS specific TCB parts
		lua_rtos_tcb->threadid = id;		
	}

	exit_critical_section();
}

UBaseType_t uxGetThreadId() {
	struct lua_rtos_tcb *lua_rtos_tcb;
	int threadid = 0;

	enter_critical_section();

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Get current thread od from Lua RTOS specific TCB parts
		threadid = lua_rtos_tcb->threadid;
	}

	exit_critical_section();

	return threadid;
}

void uxSetLuaState(lua_State* L) {
	struct lua_rtos_tcb *lua_rtos_tcb;

	enter_critical_section();

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Store current lua state into Lua RTOS specific TCB parts
		lua_rtos_tcb->L = L;		
	}

	exit_critical_section();
}

lua_State* pvGetLuaState() {
	struct lua_rtos_tcb *lua_rtos_tcb;
	lua_State *L = NULL;

	enter_critical_section();

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_POINTER_ID))) {

		// Get current lua state from Lua RTOS specific TCB parts
		L = lua_rtos_tcb->L;
	}

	exit_critical_section();

	return L;
}

uint32_t uxGetSignaled(TaskHandle_t h) {
	struct lua_rtos_tcb *lua_rtos_tcb;
	int signaled = 0;

	enter_critical_section();

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Get current signeled mask from Lua RTOS specific TCB parts
		signaled = lua_rtos_tcb->signaled;
	}

	exit_critical_section();

	return signaled;
}

void uxSetSignaled(TaskHandle_t h, int s) {
	struct lua_rtos_tcb *lua_rtos_tcb;

	enter_critical_section();

	// Get Lua RTOS specific TCB parts for current task
	if ((lua_rtos_tcb = pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_POINTER_ID))) {
		// Store current signaled mask into Lua RTOS specific TCB parts
		lua_rtos_tcb->signaled = s;		
	}

	exit_critical_section();
}

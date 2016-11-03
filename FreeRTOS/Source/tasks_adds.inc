/*
 * Lua RTOS, FreeRTOS tasks adds
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

//static int _____errno; // Errno variable if scheluder is not started

void enter_critical_section() {
    if (portIN_ISR()) {
		#ifdef PLATFORM_ESP8266
		taskDISABLE_INTERRUPTS();
		#endif

		#ifdef PLATFORM_PIC32MZ
		taskENTER_CRITICAL_FROM_ISR();
		#endif
    } else {
        taskENTER_CRITICAL();            
    }
}

void exit_critical_section() {
    if (portIN_ISR()) {
		#ifdef PLATFORM_ESP8266
        taskENABLE_INTERRUPTS();
		#endif

		#ifdef PLATFORM_PIC32MZ
		taskENTER_CRITICAL_FROM_ISR();
		#endif
    } else {
        taskEXIT_CRITICAL();            
    }        
}

//void *GetErrnoRef (void) {
//    void *ref;
//    
//    enter_critical_section();
//    if (pxCurrentTCB) {
//        ref = &(pxCurrentTCB->errno);        
//    } else {
//        ref = &_____errno;
//    }
//    exit_critical_section();
//
//    return ref;
//}

UBaseType_t uxGetTaskId() {
    UBaseType_t uxTaskNumber;
    
    enter_critical_section();
	if (pxCurrentTCB) {
		uxTaskNumber = pxCurrentTCB->uxTaskNumber;
	} else {
		uxTaskNumber = 0;
	} 
    exit_critical_section();

    return uxTaskNumber;
}

UBaseType_t uxGetThreadId() {
    int threadid;

    enter_critical_section();
	if (pxCurrentTCB) {
		threadid = pxCurrentTCB->threadid;
	} else {
		threadid = 0;
	}
    exit_critical_section();
    
    return threadid;
}

void uxSetThreadId(UBaseType_t id) {
    enter_critical_section();
	if (pxCurrentTCB) {
		pxCurrentTCB->threadid = id;
	}
    exit_critical_section();
}

void uxSetLuaState(lua_State* L) {
    enter_critical_section();
	if (pxCurrentTCB) {
		pxCurrentTCB->L = L;
	}
    exit_critical_section();
}

lua_State* pvGetLuaState() {
    lua_State* L;
    
    enter_critical_section();
	if (pxCurrentTCB) {
		L = pxCurrentTCB->L;
	} else {
		L = NULL;
	}
    exit_critical_section();
    
    return L;
}

uint32_t uxGetSignaled(TaskHandle_t h) {
	uint32_t signaled;
	
    enter_critical_section();
	if (((TCB_t *)h)) {
		signaled = ((TCB_t *)h)->signaled;
	} else {
		signaled = 0;
	}
    exit_critical_section();

	return signaled;
}

void uxSetSignaled(TaskHandle_t h, int s) {
    enter_critical_section();
	if (((TCB_t *)h)) {
		((TCB_t *)h)->signaled |= (1 << s);
	}
    exit_critical_section();
}

void uxClearSignaled(TaskHandle_t h, int s) {
    enter_critical_section();
	if (((TCB_t *)h)) {
		((TCB_t *)h)->signaled &= ~(1 << s);
	}
    exit_critical_section();
}

TaskHandle_t xGetCurrentTask() {
    return (TaskHandle_t)pxCurrentTCB;
}

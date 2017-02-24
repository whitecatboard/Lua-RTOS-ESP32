/*
 * Lua RTOS, mutex api implementation over FreeRTOS
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

#include "freertos/FreeRTOS.h"
#include "freertos/adds.h"

#include "esp_attr.h"

#include <sys/mutex.h>
#include <sys/panic.h>

extern unsigned port_interruptNesting[portNUM_PROCESSORS];

#if !MTX_USE_EVENTS

void _mtx_init() {
}

void mtx_init(struct mtx *mutex, const char *name, const char *type, int opts) {    
    mutex->sem = xSemaphoreCreateBinary();

    if (mutex->sem) {
        if (port_interruptNesting[xPortGetCoreID()] != 0) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR( mutex->sem, &xHigherPriorityTaskWoken); 
            portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
        } else {
            xSemaphoreGive( mutex->sem );            
        }
    }    
}

void IRAM_ATTR mtx_lock(struct mtx *mutex) {
    if (port_interruptNesting[xPortGetCoreID()] != 0) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;        
        xSemaphoreTakeFromISR( mutex->sem, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else {
        xSemaphoreTake( mutex->sem, portMAX_DELAY );
    }
}

int mtx_trylock(struct	mtx *mutex) {
    if (xSemaphoreTake( mutex->sem, 0 ) == pdTRUE) {
        return 1;
    } else {
        return 0;
    }
}

void IRAM_ATTR mtx_unlock(struct mtx *mutex) {
    if (port_interruptNesting[xPortGetCoreID()] != 0) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;  
        xSemaphoreGiveFromISR( mutex->sem, &xHigherPriorityTaskWoken );  
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else {
        xSemaphoreGive( mutex->sem );    
    }
}

void mtx_destroy(struct	mtx *mutex) {
    if (port_interruptNesting[xPortGetCoreID()] != 0) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;  
        xSemaphoreGiveFromISR( mutex->sem, &xHigherPriorityTaskWoken );  
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else {
        xSemaphoreGive( mutex->sem );        
    }
    
    vSemaphoreDelete( mutex->sem );
    
    mutex->sem = 0;
}

#else

// This array contains the requiered event group object for manage a
// number of MTX_MAX mutexes
eventg_t eventg[MTX_EVENT_GROUPS];

// Init mtx control structures, it must be called prior to use any mtx function
void _mtx_init() {
	int eg;
	
	for(eg=0;eg<MTX_EVENT_GROUPS;eg++) {
		eventg[eg].used = 0;
		eventg[eg].eg = xEventGroupCreate();
	}
}

// Get the number of allocated mutexes
uint32_t mtx_num(void) {
	uint32_t num = 0;
	uint32_t used;
	int eg, bit;
	
	for(eg=0;eg<MTX_EVENT_GROUPS;eg++) {
		used = eventg[eg].used;

		for(bit = 0;bit < BITS_PER_EVENT_GROUP;bit++) {
			if ((used & (1 << bit))) {
				num++;
			} 
		}
	}
	
	return num;
}

// Search for an unused bit in the event group array and mark as used.
//
// If an unused bit is found the function returns:
//
// bits 31 to 24 contains the event group identifier, and bits 23 to 0
//
// If no bit is found function returns -1.
static uint32_t allocate_mtx() {
	uint32_t used;
	int eg, bit;
	
	// Search in each event group for an unused bit
	for(eg=0;eg<MTX_EVENT_GROUPS;eg++) {
		used = eventg[eg].used;

		for(bit = 0;bit < BITS_PER_EVENT_GROUP;bit++) {
			if (!(used & (1 << bit))) {
				// Mark as used
				eventg[eg].used |= (1 << bit);
				
				// Return the allocated bit
				return MTX_ID(eg, bit);
			} 
		}
	}
	
	// No bit found
	return -1;
}

// Mark a mutex id as unused
static void free_mtx(uint32_t mtxid) {
	MTX_EVENTG(mtxid).used &= ~MTX_EVENTG_BIT(mtxid);
}

void mtx_init(struct mtx *mutex, const char *name, const char *type, int opts) {  
	uint32_t mtxid;
	
	enter_critical_section();
    
	// Allocate the mutex
	if ((mtxid = allocate_mtx()) >= 0) {
		// Store the mutex id
		mutex->mtxid = mtxid;

		if (port_interruptNesting[xPortGetCoreID()] != 0) {
		    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  
	    	xEventGroupSetBitsFromISR(MTX_EVENTG(mtxid).eg, MTX_EVENTG_BIT(mtxid),&xHigherPriorityTaskWoken);
	        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );    	
        } else {
        	xEventGroupSetBits(MTX_EVENTG(mtxid).eg, MTX_EVENTG_BIT(mtxid));  	
        }
	}

	exit_critical_section();
}

void mtx_lock(struct mtx *mutex) {	
	enter_critical_section();

	configASSERT(MTX_EVENTG_ID(mutex->mtxid) >= 0);
	configASSERT(MTX_EVENTG_ID(mutex->mtxid) <= MTX_EVENT_GROUPS);
	configASSERT(MTX_EVENTG_BIT(mutex->mtxid) <= 0x00ffffff);
		
  	xEventGroupWaitBits(MTX_EVENTG(mutex->mtxid).eg, MTX_EVENTG_BIT(mutex->mtxid), pdTRUE, pdTRUE, portMAX_DELAY);

	exit_critical_section();
}

int mtx_trylock(struct	mtx *mutex) {
	enter_critical_section();

	uint32_t mtxid = mutex->mtxid;

	configASSERT(MTX_EVENTG_ID(mtxid) >= 0);
	configASSERT(MTX_EVENTG_ID(mtxid) <= MTX_EVENT_GROUPS);
	configASSERT(MTX_EVENTG_BIT(mtxid) <= 0x00ffffff);

  	EventBits_t uxBits = xEventGroupWaitBits(MTX_EVENTG(mtxid).eg, MTX_EVENTG_BIT(mtxid), pdTRUE, pdTRUE, 0);
  	if (!(uxBits & MTX_EVENTG_BIT(mtxid))) {
  		exit_critical_section();
  		return 0;
  	}
  	
	exit_critical_section();
	
  	return 1;
}

void mtx_unlock(struct mtx *mutex) {
	uint32_t mtxid = mutex->mtxid;

	enter_critical_section();

	configASSERT(MTX_EVENTG_ID(mtxid) >= 0);
	configASSERT(MTX_EVENTG_ID(mtxid) <= MTX_EVENT_GROUPS);
	configASSERT(MTX_EVENTG_BIT(mtxid) <= 0x00ffffff);

	if (port_interruptNesting[xPortGetCoreID()] != 0) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;  
    	xEventGroupSetBitsFromISR(MTX_EVENTG(mtxid).eg, MTX_EVENTG_BIT(mtxid),&xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );    	
    } else {
    	xEventGroupSetBits(MTX_EVENTG(mtxid).eg, MTX_EVENTG_BIT(mtxid));
    }

	exit_critical_section();
}

void mtx_destroy(struct	mtx *mutex) {
	enter_critical_section();

	if (mutex->mtxid < 0) {
		exit_critical_section();
		return;
	}
	
	configASSERT(MTX_EVENTG_ID(mutex->mtxid) <= MTX_EVENT_GROUPS);
	configASSERT(MTX_EVENTG_BIT(mutex->mtxid) <= 0x00ffffff);

	free_mtx(mutex->mtxid);	

	exit_critical_section();
}

#endif

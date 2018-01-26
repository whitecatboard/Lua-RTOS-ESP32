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
 * Lua RTOS mutex api implementation over FreeRTOS
 *
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/adds.h"

#include "esp_attr.h"

#include <sys/mutex.h>
#include <sys/panic.h>

#if !CONFIG_LUA_RTOS_USE_EVENT_GROUP_IN_MTX
void _mtx_init() {
}

void mtx_init(struct mtx *mutex, const char *name, const char *type, int opts) {    
    mutex->lock = xSemaphoreCreateBinary();
    if (mutex->lock) {
        if (xPortInIsrContext()) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR( mutex->lock, &xHigherPriorityTaskWoken);
            portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
        } else {
            xSemaphoreGive( mutex->lock );
        }
    }    
}

void IRAM_ATTR mtx_lock(struct mtx *mutex) {
	if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;        
        xSemaphoreTakeFromISR( mutex->lock, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else {
        xSemaphoreTake( mutex->lock, portMAX_DELAY );
    }
}

int mtx_trylock(struct mtx *mutex) {
    if (xSemaphoreTake( mutex->lock, 0 ) == pdTRUE) {
        return 1;
    } else {
        return 0;
    }
}

void IRAM_ATTR mtx_unlock(struct mtx *mutex) {
	if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;  
        xSemaphoreGiveFromISR( mutex->lock, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else {
        xSemaphoreGive( mutex->lock );
    }
}

void mtx_destroy(struct mtx *mutex) {
	if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;  
        xSemaphoreGiveFromISR( mutex->lock, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else {
        xSemaphoreGive( mutex->lock );
    }
    
    vSemaphoreDelete( mutex->lock );
    
    mutex->lock = 0;
}
#else
// This array contains the requiered event group object for manage a
// number of MTX_MAX mutexes
static eventg_t eventg[MTX_EVENT_GROUPS];

#define mtx_lock_internal() xEventGroupWaitBits(MTX_EVENTG(1).eg, MTX_EVENTG_BIT(1), pdTRUE, pdTRUE, portMAX_DELAY);
#define mtx_unlock_internal() xEventGroupSetBits(MTX_EVENTG(1).eg, MTX_EVENTG_BIT(1));

// Search for an unused bit in the event group array and mark as used.
//
// If an unused bit is found the function returns:
//
// bits 31 to 24 contains the event group identifier, and bits 23 to 0
static uint32_t allocate_mtx() {
	uint32_t used, allocated;
	int eg, bit;

	allocated = 0;

	mtx_lock_internal();

	// Search in each event group for an unused bit
	for(eg=0;eg<MTX_EVENT_GROUPS;eg++) {
		used = eventg[eg].used;

		for(bit = 0;bit < BITS_PER_EVENT_GROUP;bit++) {
			if (!(used & (1 << bit))) {
				// Mark as used
				eventg[eg].used |= (1 << bit);
				allocated =  MTX_ID(eg, bit);
				break;
			}
		}

		if (allocated) {
			break;
		}
	}

	mtx_unlock_internal();

	assert(allocated != 0);

	return allocated;
}

// Init mtx control structures, it must be called prior to use any mtx function
void _mtx_init() {
	int eg;

	for(eg=0;eg<MTX_EVENT_GROUPS;eg++) {
		eventg[eg].used = 0;
		if (eg == 0) {
			// Reserve first mutext for internal protection
			eventg[eg].used = 0x01;
		}

		eventg[eg].eg = xEventGroupCreate();
	}
}

// Mark a mutex id as unused
static void free_mtx(uint32_t mtxid) {
	mtx_lock_internal();
	MTX_EVENTG(mtxid).used &= ~MTX_EVENTG_BIT(mtxid);
	mtx_unlock_internal();
}

void mtx_init(struct mtx *mutex, const char *name, const char *type, int opts) {
	int32_t mtxid;

	assert(mutex != NULL);

	// Allocate the mutex
	mtxid = allocate_mtx();

	// Store the mutex id
	mutex->lock = (uint32_t)mtxid;

	if (xPortInIsrContext()) {
		xEventGroupSetBitsFromISR(MTX_EVENTG(mtxid).eg, MTX_EVENTG_BIT(mtxid),NULL);
	} else {
		xEventGroupSetBits(MTX_EVENTG(mtxid).eg, MTX_EVENTG_BIT(mtxid));
	}
}

void mtx_lock(struct mtx *mutex) {
	assert(mutex != NULL);
	if (xPortInIsrContext()) return;

	xEventGroupWaitBits(MTX_EVENTG(mutex->lock).eg, MTX_EVENTG_BIT(mutex->lock), pdTRUE, pdTRUE, portMAX_DELAY);
}

int mtx_trylock(struct mtx *mutex) {
	assert(mutex != NULL);
	if (xPortInIsrContext()) return 1;

	uint32_t mtxid = mutex->lock;

  	EventBits_t uxBits = xEventGroupWaitBits(MTX_EVENTG(mtxid).eg, MTX_EVENTG_BIT(mtxid), pdTRUE, pdTRUE, 0);
  	if (!(uxBits & MTX_EVENTG_BIT(mtxid))) {
  		return 0;
  	}

  	return 1;
}

void mtx_unlock(struct mtx *mutex) {
	assert(mutex != NULL);
	if (xPortInIsrContext()) return;

	uint32_t mtxid = mutex->lock;

   	xEventGroupSetBits(MTX_EVENTG(mtxid).eg, MTX_EVENTG_BIT(mtxid));
}

void mtx_destroy(struct mtx *mutex) {
	assert(mutex != NULL);

	free_mtx(mutex->lock);
}
#endif

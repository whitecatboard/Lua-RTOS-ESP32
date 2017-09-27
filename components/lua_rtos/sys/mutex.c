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

void mtx_init(struct mtx *mutex, const char *name, const char *type, int opts) {    
    mutex->sem = xSemaphoreCreateBinary();
    if (mutex->sem) {
        if (xPortInIsrContext()) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR( mutex->sem, &xHigherPriorityTaskWoken); 
            portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
        } else {
            xSemaphoreGive( mutex->sem );            
        }
    }    
}

void IRAM_ATTR mtx_lock(struct mtx *mutex) {
	if (xPortInIsrContext()) {
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
	if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;  
        xSemaphoreGiveFromISR( mutex->sem, &xHigherPriorityTaskWoken );  
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else {
        xSemaphoreGive( mutex->sem );    
    }
}

void mtx_destroy(struct	mtx *mutex) {
	if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;  
        xSemaphoreGiveFromISR( mutex->sem, &xHigherPriorityTaskWoken );  
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else {
        xSemaphoreGive( mutex->sem );        
    }
    
    vSemaphoreDelete( mutex->sem );
    
    mutex->sem = 0;
}

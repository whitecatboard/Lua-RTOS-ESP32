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

int mtx_inited(struct mtx *mutex) {
    return (mutex->lock != 0);
}

void mtx_init(struct mtx *mutex, const char *name, const char *type, int opts) {
    mutex->opts = opts;

    if (mutex->opts == MTX_DEF) {
        mutex->lock = xSemaphoreCreateBinary();
    } else if (opts == MTX_RECURSE) {
        mutex->lock = xSemaphoreCreateRecursiveMutex();
    } else {
        return;
    }

    if (mutex->lock) {
        if (xPortInIsrContext()) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR( mutex->lock, &xHigherPriorityTaskWoken);
            portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
        } else {
            if (mutex->opts == MTX_DEF) {
                xSemaphoreGive( mutex->lock );
            }
        }
    }    
}

void IRAM_ATTR mtx_lock(struct mtx *mutex) {
    if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreTakeFromISR( mutex->lock, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else {
        if (mutex->opts == MTX_DEF) {
            xSemaphoreTake( mutex->lock, portMAX_DELAY );
        } else if (mutex->opts == MTX_RECURSE) {
            xSemaphoreTakeRecursive( mutex->lock, portMAX_DELAY );
        }
    }
}

int mtx_trylock(struct mtx *mutex) {
    if (mutex->opts == MTX_DEF) {
        if (xSemaphoreTake( mutex->lock, 0 ) == pdTRUE) {
            return 1;
        } else {
            return 0;
        }
    } else if (mutex->opts == MTX_RECURSE) {
        if (xSemaphoreTakeRecursive( mutex->lock, 0 ) == pdTRUE) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}

void IRAM_ATTR mtx_unlock(struct mtx *mutex) {
    if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;  
        xSemaphoreGiveFromISR( mutex->lock, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else {
        if (mutex->opts == MTX_DEF) {
            xSemaphoreGive( mutex->lock );
        } else if (mutex->opts == MTX_RECURSE) {
            xSemaphoreGiveRecursive( mutex->lock );
        }
    }
}

void mtx_destroy(struct mtx *mutex) {
    if (!mutex->lock) return;

    if (mutex->opts == MTX_DEF) {
        if (xPortInIsrContext()) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR( mutex->lock, &xHigherPriorityTaskWoken );
            portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
        } else {
            xSemaphoreGive( mutex->lock );
        }

        vSemaphoreDelete( mutex->lock );
    } else if (mutex->opts == MTX_RECURSE) {
        vSemaphoreDelete( mutex->lock );
    }

    mutex->lock = 0;
}

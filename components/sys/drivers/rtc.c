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
 * Lua RTOS, RTC driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_RTC

#include "esp_attr.h"
#include "esp_sleep.h"

#include "rom/rtc.h"

#include <string.h>

#include <sys/driver.h>
#include <sys/mutex.h>

#include <sys/drivers/rtc.h>
#include <sys/drivers/cpu.h>

// Defined in linker
extern uint32_t _rtc_force_slow_end;

// Mutex to protect concurrent access
static struct mtx mtx;

// Registered meta types sizes
static uint8_t metas[16];

/*
 * In Lua RTOS, RTC memory is organized as a stack, in which the programmer can
 * push / pop values of a certain data types. The fact that RTC can be used by
 * high level languages like Lua, meta data information about the stack's content
 * also is stored.
 *
 * To manage the stack, 2 pointers are used:
 *
 * - rtc_data_p: pointer to the top of the stack, this pointer is incremented when
 *               a value is pushed, and decremented when a value is popped.
 *
 * - rtc_meta_p: pointer to the top of the stack meta data, in which each byte store
 *               meta data of 2 stack values (4 bits per value).
 *
 *
 * rtc_data_p is located into the low RTC memory region, and rtc_meta_p is located
 * into the high RTC memory region.
 *
 */
RTC_DATA_ATTR static uint8_t *rtc_data_p;
RTC_DATA_ATTR static uint8_t *rtc_meta_p;

static uint8_t *rtc_data_start;
static uint8_t *rtc_meta_start;

static void __rtc_init();

// Register driver and messages
DRIVER_REGISTER_BEGIN(RTC,rtc,NULL,__rtc_init,NULL);
    DRIVER_REGISTER_ERROR(RTC, rtc, NotEnoughMemory, "not enough memory", RTC_ERR_NOT_ENOUGH_MEMORY);
    DRIVER_REGISTER_ERROR(RTC, rtc, Empty, "empty memory", RTC_ERR_EMPTY_MEMORY);
    DRIVER_REGISTER_ERROR(RTC, rtc, TypeNotAllowed, "type not allowed", RTC_ERR_TYPE_NOT_ALLOWED);
DRIVER_REGISTER_END(RTC,rtc,NULL,__rtc_init,NULL);

/*
 * Helper functions
 */

static void __rtc_init() {
    size_t size = rtc_mem_size();

    rtc_data_start = (uint8_t *)(&_rtc_force_slow_end + sizeof(uint32_t));
    rtc_meta_start = rtc_data_start + size - 1;

    // Get reset reason
    int reason = cpu_reset_reason();

    if (reason != DEEPSLEEP_RESET) {
        // Initialize stack pointers
        rtc_data_p = (uint8_t *)(&_rtc_force_slow_end + sizeof(uint32_t));
        rtc_meta_p = rtc_data_p + size - 1;

        // Initialize RTC memory
        memset(rtc_data_p, 0, size);
    }

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);

    // Create mutex
    mtx_init(&mtx, NULL, NULL, 0);
}

size_t rtc_mem_size() {
    return (0x50000000 + 0x1000 - (uint32_t)&_rtc_force_slow_end) - sizeof(uint32_t);
}

size_t rtc_mem_free() {
    return rtc_meta_p - rtc_data_p + 1;
}

void rtc_mem_register_meta(uint8_t meta, size_t size) {
    metas[meta] = size;
}

size_t rtc_mem_get_meta_size(uint8_t meta) {
    return metas[meta];
}

driver_error_t *rtc_mem_push(uint8_t meta, void *val) {
    size_t size = metas[meta];

    mtx_lock(&mtx);

    // We need to store the meta data at the top of the meta data stack. They are
    // 3 cases:
    //
    // * The current top has space for the meta data if one of it's nibbles are 0
    // * The current top has not space for the meta data, so we need to decrement
    //   the top of the stack
    //
    uint8_t *tmp_rtc_meta_p = rtc_meta_p;
    uint8_t tmp_meta = *tmp_rtc_meta_p;

    if ((tmp_meta & 0x0f) == 0) {
        tmp_meta = meta;
    } else if ((tmp_meta & 0xf0) == 0) {
        tmp_meta |= (meta << 4);
    } else {
        tmp_rtc_meta_p--;
        tmp_meta = meta;
    }

    // Store element value and it's meta data
    if (size > 0) {
        if (tmp_rtc_meta_p > rtc_data_p + size) {
            *tmp_rtc_meta_p = tmp_meta;
            rtc_meta_p = tmp_rtc_meta_p;
            memcpy(rtc_data_p, val, size);
            rtc_data_p += size;
            assert(rtc_data_p < rtc_meta_p);
        } else {
            mtx_unlock(&mtx);

            return driver_error(RTC_DRIVER, RTC_ERR_NOT_ENOUGH_MEMORY, NULL);
        }
    } else {
        *tmp_rtc_meta_p = tmp_meta;
        rtc_meta_p = tmp_rtc_meta_p;
    }

    mtx_unlock(&mtx);

    return NULL;
}

driver_error_t *rtc_mem_pop(uint8_t *meta, size_t *size, void **val) {
    uint8_t *tmp_rtc_meta_p = rtc_meta_p;

    mtx_lock(&mtx);

    // Get meta data from the top of the meta data stack
    uint8_t tmp_meta = *tmp_rtc_meta_p;

    if ((tmp_meta & 0xf0) != 0) {
        *meta = (tmp_meta & 0xf0) >> 4;
        tmp_meta = tmp_meta & 0x0f;
    } else if ((tmp_meta & 0x0f) != 0) {
        *meta = tmp_meta;
        tmp_meta = 0x00;

        if (tmp_rtc_meta_p + 1 <= rtc_meta_start) {
            tmp_rtc_meta_p++;
        }
    } else {
        mtx_unlock(&mtx);
        return driver_error(RTC_DRIVER, RTC_ERR_EMPTY_MEMORY, NULL);
    }

    // Get meta data size
    *size = metas[*meta];

    if (*size > 0) {
        // Allocate space to return value
        *val = calloc(1, *size);
        if (!*val) {
            mtx_unlock(&mtx);

            return driver_error(RTC_DRIVER, RTC_ERR_NOT_ENOUGH_MEMORY, NULL);
        }
    }

    // Update meta data top
    *rtc_meta_p = tmp_meta;
    rtc_meta_p = tmp_rtc_meta_p;

    if (*size > 0) {
        // Update data top
        rtc_data_p -= *size;
        assert(rtc_data_p >= rtc_data_start);

        // Return value
        memcpy(*val, rtc_data_p, *size);
    }

    mtx_unlock(&mtx);

    return NULL;
}

#endif

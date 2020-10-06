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
 * Lua RTOS status management
 *
 */

#include <sys/mutex.h>
#include <sys/status.h>

static struct mtx mtx;               // Mutex for protect the status
static uint32_t LuaRTOS_status;      // Current status
static uint32_t LuaRTOS_prev_status; // Previous status

void _status_init() {
    mtx_init(&mtx, NULL, NULL, 0);
    LuaRTOS_status = 0;
    LuaRTOS_prev_status = 0;
}

void IRAM_ATTR status_set(uint32_t setMask, uint32_t clearMask) {
    mtx_lock(&mtx);
    LuaRTOS_prev_status = LuaRTOS_status;
    LuaRTOS_status |= setMask;
    LuaRTOS_status &= ~clearMask;
    mtx_unlock(&mtx);
}

int IRAM_ATTR status_get(uint32_t mask) {
    mtx_lock(&mtx);
    int res = ((LuaRTOS_status & mask) == mask);
    mtx_unlock(&mtx);

    return res;
}

int IRAM_ATTR status_get_prev(uint32_t mask) {
    mtx_lock(&mtx);
    int res = ((LuaRTOS_prev_status & mask) == mask);
    mtx_unlock(&mtx);

    return res;
}

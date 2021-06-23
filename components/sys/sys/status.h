/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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

#ifndef _SYS_STATUS_H_
#define _SYS_STATUS_H_

#include "esp_attr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/adds.h"

#include <stdint.h>

#define NETWORK_AVAILABLE() (\
	(status_get(STATUS_WIFI_CONNECTED) && status_get(STATUS_WIFI_HAS_IP)) ||\
	(status_get(STATUS_SPI_ETH_CONNECTED) && status_get(STATUS_SPI_ETH_HAS_IP)) ||\
	(status_get(STATUS_ETH_CONNECTED) && status_get(STATUS_ETH_HAS_IP))\
)

#define STATUS_SYSCALLS_INITED         (1 <<  0)
#define STATUS_LUA_RUNNING             (1 <<  1)
#define STATUS_LUA_INTERPRETER         (1 <<  2)
#define STATUS_LUA_ABORT_BOOT_SCRIPTS  (1 <<  3)
#define STATUS_LUA_HISTORY             (1 <<  4)
#define STATUS_LUA_SHELL               (1 <<  5)
#define STATUS_NEED_RTC_SLOW_MEM       (1 <<  6)
#define STATUS_ISR_SERVICE_INSTALLED   (1 <<  7)
#define STATUS_TCPIP_INITED            (1 <<  8)
#define STATUS_WIFI_INITED             (1 <<  9)
#define STATUS_WIFI_SETUP              (1 << 10)
#define STATUS_WIFI_SYNC               (1 << 11)
#define STATUS_WIFI_STARTED            (1 << 12)
#define STATUS_WIFI_CONNECTED          (1 << 13)
#define STATUS_WIFI_HAS_IP             (1 << 14)
#define STATUS_SPI_ETH_SETUP           (1 << 15)
#define STATUS_SPI_ETH_SYNC            (1 << 16)
#define STATUS_SPI_ETH_STARTED         (1 << 17)
#define STATUS_SPI_ETH_CONNECTED       (1 << 18)
#define STATUS_SPI_ETH_HAS_IP          (1 << 19)
#define STATUS_ETH_SETUP               (1 << 20)
#define STATUS_ETH_SYNC                (1 << 21)
#define STATUS_ETH_STARTED             (1 << 22)
#define STATUS_ETH_CONNECTED           (1 << 23)
#define STATUS_ETH_HAS_IP              (1 << 24)

void IRAM_ATTR status_set(uint32_t setMask, uint32_t clearMask);
int  IRAM_ATTR status_get(uint32_t mask);
int  IRAM_ATTR status_get_prev(uint32_t mask);

#endif /* !_SYS_STATUS_H_ */

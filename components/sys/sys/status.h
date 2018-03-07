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

#ifndef _SYS_STATUS_H_
#define _SYS_STATUS_H_

#include "esp_attr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/adds.h"

#include <stdint.h>

#define NETWORK_AVAILABLE() (status_get(STATUS_WIFI_CONNECTED) || status_get(STATUS_SPI_ETH_CONNECTED) || status_get(STATUS_ETH_CONNECTED))

#define STATUS_SYSCALLS_INITED	       0x0000
#define STATUS_LUA_RUNNING			   0x0001
#define STATUS_LUA_INTERPRETER  	   0x0002
#define STATUS_LUA_ABORT_BOOT_SCRIPTS  0x0003
#define STATUS_LUA_HISTORY			   0x0004
#define STATUS_LUA_SHELL			   0x0005
#define STATUS_TCPIP_INITED            0x0006
#define STATUS_WIFI_INITED             0x0007
#define STATUS_WIFI_SETUP              0x0008
#define STATUS_WIFI_STARTED            0x0009
#define STATUS_WIFI_CONNECTED          0x000a
#define STATUS_NEED_RTC_SLOW_MEM       0x000b
#define STATUS_ISR_SERVICE_INSTALLED   0x000c
#define STATUS_SPI_ETH_SETUP           0x000d
#define STATUS_SPI_ETH_STARTED         0x000e
#define STATUS_SPI_ETH_CONNECTED       0x000f
#define STATUS_ETH_SETUP           	   0x0010
#define STATUS_ETH_STARTED         	   0x0011
#define STATUS_ETH_CONNECTED           0x0012

extern uint32_t LuaRTOS_status[];

void IRAM_ATTR status_set(uint16_t flag);
void IRAM_ATTR status_clear(uint16_t flag);
int  IRAM_ATTR status_get(uint16_t flag);

#endif /* !_SYS_STATUS_H_ */

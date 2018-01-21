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
 * Lua RTOS, spi ethernet driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET && CONFIG_LUA_RTOS_ETH_HW_TYPE_SPI

#ifndef _SPI_ETH_
#define _SPI_ETH_

#include "net.h"

#include "esp_wifi.h"
#include "tcpip_adapter.h"

#include <sys/driver.h>

// SPI ethernet errors
#define SPI_ETH_ERR_CANT_INIT              (DRIVER_EXCEPTION_BASE(SPI_ETH_DRIVER_ID) |  0)
#define SPI_ETH_ERR_NOT_INIT               (DRIVER_EXCEPTION_BASE(SPI_ETH_DRIVER_ID) |  1)
#define SPI_ETH_ERR_NOT_START              (DRIVER_EXCEPTION_BASE(SPI_ETH_DRIVER_ID) |  2)
#define SPI_ETH_ERR_CANT_CONNECT           (DRIVER_EXCEPTION_BASE(SPI_ETH_DRIVER_ID) |  3)

extern const int spi_eth_errors;
extern const int spi_eth_error_map;

driver_error_t *spi_eth_setup(uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns1, uint32_t dns2);
driver_error_t *spi_eth_start();
driver_error_t *spi_eth_stop();
driver_error_t *spi_eth_stat(ifconfig_t *info);

#endif

#endif

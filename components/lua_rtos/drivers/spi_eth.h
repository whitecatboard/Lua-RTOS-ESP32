/*
 * Lua RTOS, SPI ethernet driver
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET && CONFIG_SPI_ETHERNET

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

driver_error_t *spi_eth_setup(uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns1, uint32_t dns2);
driver_error_t *spi_eth_start();
driver_error_t *spi_eth_stop();
driver_error_t *spi_eth_stat(ifconfig_t *info);

#endif

#endif

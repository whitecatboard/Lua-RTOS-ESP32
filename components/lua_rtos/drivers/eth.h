/*
 * Lua RTOS, RMII Ethernet driver
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

#ifndef DRIVERS_ETH_H_
#define DRIVERS_ETH_H_

#include <sys/driver.h>

#include <drivers/net.h>

// SPI ethernet errors
#define ETH_ERR_CANT_INIT              (DRIVER_EXCEPTION_BASE(ETH_DRIVER_ID) |  0)
#define ETH_ERR_NOT_INIT               (DRIVER_EXCEPTION_BASE(ETH_DRIVER_ID) |  1)
#define ETH_ERR_NOT_START              (DRIVER_EXCEPTION_BASE(ETH_DRIVER_ID) |  2)
#define ETH_ERR_CANT_CONNECT           (DRIVER_EXCEPTION_BASE(ETH_DRIVER_ID) |  3)

extern const int eth_errors;
extern const int eth_error_map;

driver_error_t *eth_setup(uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns1, uint32_t dns2);
driver_error_t *eth_start();
driver_error_t *eth_stop();
driver_error_t *eth_stat(ifconfig_t *info);

#endif /* DRIVERS_ETH_H_ */

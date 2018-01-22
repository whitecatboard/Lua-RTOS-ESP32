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
 * Lua RTOS, RMII ethernet driver
 *
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
driver_error_t *eth_start(uint8_t silent);
driver_error_t *eth_stop();
driver_error_t *eth_stat(ifconfig_t *info);

#endif /* DRIVERS_ETH_H_ */

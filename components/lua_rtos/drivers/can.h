/*
 * Lua RTOS, CAN driver
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

#ifndef CAN_H
#define	CAN_H

#include "sdkconfig.h"

#include <stdint.h>

#include <sys/driver.h>

// Get the TX GPIO from Kconfig
#if CONFIG_LUA_RTOS_CAN_TX_GPIO5
#define CONFIG_LUA_RTOS_CAN_TX 5
#endif

#if CONFIG_LUA_RTOS_CAN_TX_GPIO12
#define CONFIG_LUA_RTOS_CAN_TX 12
#endif

#if CONFIG_LUA_RTOS_CAN_TX_GPIO25
#define CONFIG_LUA_RTOS_CAN_TX 25
#endif

// Get the RX GPIO from Kconfig
#if CONFIG_LUA_RTOS_CAN_RX_GPIO4
#define CONFIG_LUA_RTOS_CAN_RX 4
#endif

#if CONFIG_LUA_RTOS_CAN_RX_GPIO14
#define CONFIG_LUA_RTOS_CAN_RX 14
#endif

#if CONFIG_LUA_RTOS_CAN_RX_GPIO35
#define CONFIG_LUA_RTOS_CAN_RX 35
#endif

// CAN errors
#define CAN_ERR_NOT_ENOUGH_MEMORY           (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  0)
#define CAN_ERR_INVALID_FRAME_LENGTH		(DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  1)

driver_error_t *can_setup(uint32_t unit, uint16_t speed);
driver_error_t *can_tx(uint32_t unit, uint32_t msg_id, uint8_t msg_type, uint8_t *data, uint8_t len);
driver_error_t *can_rx(uint32_t unit, uint32_t *msg_id, uint8_t *msg_type, uint8_t *data, uint8_t *len);

#endif	/* CAN_H */

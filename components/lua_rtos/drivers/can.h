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
#include <pthread.h>

#include <sys/driver.h>

#define CAN_NUM_FILTERS 10

typedef struct {
	int32_t fromID;
	int32_t toID;
} CAN_filter_t;

typedef struct {
	uint32_t port;
	int socket;
	int client;
	uint8_t unit;
	uint8_t stop;
	pthread_t thread;
} can_gw_config_t;

#define CAN_MAX_DLEN 8

typedef uint32_t canid_t;

struct can_frame {
	canid_t    can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
	uint8_t    can_dlc; /* frame payload length in byte (0 .. CAN_MAX_DLEN) */
	uint8_t    __pad;   /* padding */
	uint8_t    __res0;  /* reserved / padding */
	uint8_t    __res1;  /* reserved / padding */
	uint8_t    data[CAN_MAX_DLEN] __attribute__((aligned(8)));
};

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
#define CAN_ERR_INVALID_UNIT				(DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  2)
#define CAN_ERR_NO_MORE_FILTERS_ALLOWED		(DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  3)
#define CAN_ERR_INVALID_FILTER			    (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  4)
#define CAN_ERR_IS_NOT_SETUP		   	    (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  5)
#define CAN_ERR_CANT_START		   	    	(DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  6)
#define CAN_ERR_GW_NOT_STARTED				(DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  7)

extern const int can_errors;
extern const int can_error_map;

driver_error_t *can_setup(int32_t unit, uint16_t speed);
driver_error_t *can_tx(int32_t unit, uint32_t msg_id, uint8_t msg_type, uint8_t *data, uint8_t len);
driver_error_t *can_rx(int32_t unit, uint32_t *msg_id, uint8_t *msg_type, uint8_t *data, uint8_t *len);
driver_error_t *can_add_filter(int32_t unit, int32_t fromId, int32_t toId);
driver_error_t *can_remove_filter(int32_t unit, int32_t fromId, int32_t toId);
driver_error_t *can_gateway_start(int32_t unit, uint16_t speed, int32_t port);
driver_error_t *can_gateway_stop(int32_t unit);

#endif	/* CAN_H */

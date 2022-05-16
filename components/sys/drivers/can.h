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
 * Lua RTOS, CAN driver
 *
 */

#ifndef CAN_H
#define	CAN_H

#include "sdkconfig.h"

#include <stdint.h>
#include <pthread.h>

#include <sys/driver.h>

#define CAN_NUM_FILTERS 10

/**
 * \brief CAN frame type (standard/extended)
 */
typedef enum {
	CAN_frame_std=0, 						/**< Standard frame, using 11 bit identifer. */
	CAN_frame_ext=1 						/**< Extended frame, using 29 bit identifer. */
}CAN_frame_format_t;

/**
 * \brief CAN RTR
 */
typedef enum {
	CAN_no_RTR=0, 							/**< No RTR frame. */
	CAN_RTR=1 								/**< RTR frame. */
}CAN_RTR_t;

/** \brief Frame information record type */
typedef union{uint32_t U;					/**< \brief Unsigned access */
	 struct {
		uint8_t 			DLC:4;        	/**< \brief [3:0] DLC, Data length container */
		unsigned int 		unknown_2:2;    /**< \brief \internal unknown */
		CAN_RTR_t 			RTR:1;          /**< \brief [6:6] RTR, Remote Transmission Request */
		CAN_frame_format_t 	FF:1;           /**< \brief [7:7] Frame Format, see# CAN_frame_format_t*/
		unsigned int 		reserved_24:24;	/**< \brief \internal Reserved */
	} B;
} CAN_FIR_t;

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

/*
 * Controller Area Network Identifier structure
 *
 * bit 0-28	: CAN identifier (11/29 bit)
 * bit 29	: error message frame flag (0 = data frame, 1 = error message)
 * bit 30	: remote transmission request flag (1 = rtr frame)
 * bit 31	: frame format flag (0 = standard 11 bit, 1 = extended 29 bit)
 */
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
#define CAN_ERR_NOT_ENOUGH_MEMORY         (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  0)
#define CAN_ERR_INVALID_FRAME_LENGTH      (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  1)
#define CAN_ERR_INVALID_UNIT              (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  2)
#define CAN_ERR_NO_MORE_FILTERS_ALLOWED   (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  3)
#define CAN_ERR_INVALID_FILTER            (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  4)
#define CAN_ERR_IS_NOT_SETUP              (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  5)
#define CAN_ERR_CANT_START                (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  6)
#define CAN_ERR_GW_NOT_STARTED            (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  7)
//#define CAN_ERR_CANT_INIT               (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  0)
#define CAN_ERR_INVALID_ARGUMENT          (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  8)
#define CAN_ERR_NOT_SUPPORT               (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) |  9)
#define CAN_ERR_TRANSMIT_FAIL             (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) | 10)
#define CAN_ERR_INVALID_STATE             (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) | 11)
#define CAN_ERR_TRANSMIT_TIMEOUT          (DRIVER_EXCEPTION_BASE(CAN_DRIVER_ID) | 12)

extern const int can_errors;
extern const int can_error_map;

driver_error_t *can_check_error(esp_err_t error);
driver_error_t *can_setup(int32_t unit, uint32_t speed, uint16_t rx_size);
driver_error_t *can_tx(int32_t unit, uint32_t msg_id, uint8_t msg_type, uint8_t *data, uint8_t len);
driver_error_t *can_rx(int32_t unit, uint32_t *msg_id, uint8_t *msg_type, uint8_t *data, uint8_t *len, uint32_t timeout);
driver_error_t *can_add_filter(int32_t unit, int32_t fromId, int32_t toId);
driver_error_t *can_remove_filter(int32_t unit, int32_t fromId, int32_t toId);
driver_error_t *can_gateway_start(int32_t unit, uint32_t speed, int32_t port);
driver_error_t *can_gateway_stop(int32_t unit);

#endif	/* CAN_H */

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

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_CAN

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"

#include <stdint.h>
#include <string.h>

#include <can_bus.h>

#include <sys/driver.h>
#include <sys/syslog.h>
#include <sys/mutex.h>

#include <drivers/can.h>
#include <drivers/gpio.h>

#include <pthread.h>

static uint8_t setup = 0;
struct mtx mtx;

// CAN configuration
CAN_device_t CAN_cfg;

// CAN gateway configuration
can_gw_config_t *gw_config = NULL;

// CAN filters
static uint8_t filters = 0;
static CAN_filter_t can_filter[CAN_NUM_FILTERS];

// Register driver and errors
DRIVER_REGISTER_BEGIN(CAN,can,NULL,NULL,NULL);
	DRIVER_REGISTER_ERROR(CAN, can, NotEnoughtMemory, "not enough memory", CAN_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(CAN, can, InvalidFrameLength, "invalid frame length", CAN_ERR_INVALID_FRAME_LENGTH);
	DRIVER_REGISTER_ERROR(CAN, can, InvalidUnit, "invalid unit", CAN_ERR_INVALID_UNIT);
	DRIVER_REGISTER_ERROR(CAN, can, NoMoreFiltersAllowed, "no more filters allowed", CAN_ERR_NO_MORE_FILTERS_ALLOWED);
	DRIVER_REGISTER_ERROR(CAN, can, InvalidFilter, "invalid filter", CAN_ERR_INVALID_FILTER);
	DRIVER_REGISTER_ERROR(CAN, can, NotSetup, "is not setup", CAN_ERR_IS_NOT_SETUP);
	DRIVER_REGISTER_ERROR(CAN, can, CannotStart, "can't start", CAN_ERR_CANT_START);
	DRIVER_REGISTER_ERROR(CAN, can, GatewayNotStarted, "gateway not started", CAN_ERR_GW_NOT_STARTED);
DRIVER_REGISTER_END(CAN,can,NULL,NULL,NULL);

/*
 * Helper functions
 */

static void can_ll_tx(CAN_frame_t *frame) {
	mtx_lock(&mtx);
	CAN_write_frame(frame);
	mtx_unlock(&mtx);
}

static void can_ll_rx(CAN_frame_t *frame) {
	// Read next frame
    // Check filter
	uint8_t i;
    uint8_t pass = 0;

    while (!pass) {
    	xQueueReceive(CAN_cfg.rx_queue, frame, portMAX_DELAY);
        if (filters > 0) {
            for(i=0;((i < CAN_NUM_FILTERS) && (!pass));i++) {
            	if ((can_filter[i].fromID >= 0) && (can_filter[i].toID >= 0) && (frame->FIR.B.DLC > 0) && (frame->FIR.B.DLC < 9)) {
            		pass = ((frame->MsgID >= can_filter[i].fromID) && (frame->MsgID <= can_filter[i].toID));
            	}
            }
        } else {
        	pass = 1;
        }
    }
}

static void *gw_thread_up(void *arg) {
	CAN_frame_t frame;
	struct can_frame packet;

	// Set a timeout for send
	struct timeval tout;
	tout.tv_sec = 4;
	tout.tv_usec = 0;
	setsockopt(gw_config->client, SOL_SOCKET, SO_SNDTIMEO, &tout, sizeof(tout));

	while(!gw_config->stop) {
		// Wait for a CAN packet
		can_ll_rx(&frame);
		if (frame.FIR.B.DLC != 0) {
			// Fill packet
			memset(&packet, 0, sizeof(packet));

			packet.can_id = (frame.FIR.B.FF << 31) | (frame.FIR.B.RTR << 30) | frame.MsgID;
			packet.can_dlc = frame.FIR.B.DLC;

			memcpy(packet.data, frame.data.u8, sizeof(frame.data));

			// Write frame to socket
			if (write(gw_config->client, &packet, sizeof(packet)) != sizeof(packet)) {
				close(gw_config->client);

				syslog(LOG_ERR, "can%d gateway: can't write to socket", gw_config->unit);

				return NULL;
			}
		}
	}

	close(gw_config->client);

	return NULL;
}

static void *gw_thread_down(void *arg) {
	struct can_frame packet;
	CAN_frame_t frame;

	while(!gw_config->stop) {
		if (read(gw_config->client, &packet, sizeof(packet)) == sizeof(packet)) {
			// Fill frame
			frame.MsgID = (packet.can_id & 0b11111111111111111111111111111);
			frame.FIR.B.FF = (packet.can_id & 0b10000000000000000000000000000000?1:0);
			frame.FIR.B.RTR = (packet.can_id & 0b01000000000000000000000000000000?1:0);
			frame.FIR.B.DLC = packet.can_dlc;

			memcpy(frame.data.u8, packet.data, sizeof(packet.data));

			can_ll_tx(&frame);
		}
	}

	close(gw_config->client);

	return NULL;
}

static void *gw_thread(void *arg) {
	// Create an setup socket
	struct sockaddr_in6 sin;

	gw_config->socket = socket(AF_INET6, SOCK_STREAM, 0);
	if (gw_config->socket < 0) {
		syslog(LOG_ERR, "can%d gateway: can't create socket", gw_config->unit);
		return NULL;
	}

	// Bind and listen
	memset(&sin, 0, sizeof(sin));
	sin.sin6_family = AF_INET6;
	memcpy(&sin.sin6_addr.un.u32_addr, &in6addr_any, sizeof(in6addr_any));
	sin.sin6_port = htons(gw_config->port);

	if(bind(gw_config->socket, (struct sockaddr *) &sin, sizeof (sin))) {
		syslog(LOG_ERR, "can%d gateway: can't bind to port %d",gw_config->unit, gw_config->port);
		return NULL;
	}

	if (listen(gw_config->socket, 5)) {
		syslog(LOG_ERR, "can%d gateway: can't listen on port %d",gw_config->unit, gw_config->port);
		return NULL;
	}

	syslog(LOG_INFO, "can%d gateway: started at port %d", gw_config->unit, gw_config->port);

	while (!gw_config->stop) {
		// Wait for a connection
		gw_config->client = accept(gw_config->socket, (struct sockaddr*) NULL, NULL);
		if (gw_config->client > 0) {
			syslog(LOG_INFO, "can%d gateway: client connected", gw_config->unit);

			// Start threads for client
			pthread_t thread_up;
			pthread_t thread_down;
			pthread_attr_t attr;

			pthread_attr_init(&attr);

			if (pthread_create(&thread_up, &attr, gw_thread_up, NULL)) {
				syslog(LOG_ERR, "can%d gateway: can't start up thread", gw_config->unit);
				return NULL;
			}

			if (pthread_create(&thread_down, &attr, gw_thread_down, NULL)) {
				syslog(LOG_ERR, "can%d gateway: can't start down thread", gw_config->unit);
				return NULL;
			}

			pthread_setname_np(thread_up, "can gw up");
			pthread_setname_np(thread_down, "can gw down");

			pthread_join(thread_up, NULL);
			pthread_join(thread_down, NULL);
		}
	}

	close(gw_config->socket);

	return NULL;
}

/*
 * Operation functions
 */

driver_error_t *can_setup(int32_t unit, uint16_t speed) {
	driver_unit_lock_error_t *lock_error = NULL;

	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if (!setup) {
	    // Lock TX pin
	    if ((lock_error = driver_lock(CAN_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_CAN_TX, DRIVER_ALL_FLAGS, "TX"))) {
	    	// Revoked lock on pin
	    	return driver_lock_error(CAN_DRIVER, lock_error);
	    }

	    // Lock RX pin
	    if ((lock_error = driver_lock(CAN_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_CAN_RX, DRIVER_ALL_FLAGS, "RX"))) {
	    	// Revoked lock on pin
	    	return driver_lock_error(CAN_DRIVER, lock_error);
	    }
	}

	// Set CAN configuration
	CAN_cfg.speed = speed;
	CAN_cfg.tx_pin_id = CONFIG_LUA_RTOS_CAN_TX;
	CAN_cfg.rx_pin_id = CONFIG_LUA_RTOS_CAN_RX;

	if (!setup) {
		CAN_cfg.rx_queue = xQueueCreate(50,sizeof(CAN_frame_t));;

		// Init filters
		uint8_t i;
		for(i=0;i < CAN_NUM_FILTERS;i++) {
			can_filter[i].fromID = -1;
			can_filter[i].toID = -1;
		}

		filters = 0;

		if (!CAN_cfg.rx_queue) {
			return driver_error(CAN_DRIVER, CAN_ERR_NOT_ENOUGH_MEMORY, NULL);
		}
	}

	// Start CAN module
	CAN_init();

	if (!setup) {
	    mtx_init(&mtx, NULL, NULL, 0);

		syslog(LOG_INFO, "can%d at pins tx=%s%d, rx=%s%d", 0,
			gpio_portname(CONFIG_LUA_RTOS_CAN_TX), gpio_name(CONFIG_LUA_RTOS_CAN_TX),
			gpio_portname(CONFIG_LUA_RTOS_CAN_RX), gpio_name(CONFIG_LUA_RTOS_CAN_RX)
		);
	}

	setup = 1;

	return NULL;
}

driver_error_t *can_tx(int32_t unit, uint32_t msg_id, uint8_t msg_type, uint8_t *data, uint8_t len) {
	CAN_frame_t frame;

	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if (len > 8) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_FRAME_LENGTH, NULL);
	}

	if (!setup) {
		return driver_error(CAN_DRIVER, CAN_ERR_IS_NOT_SETUP, NULL);
	}

	// Populate frame
	frame.MsgID = msg_id;
	frame.FIR.B.DLC = len;
	memcpy(&frame.data, data, len);

	can_ll_tx(&frame);

	return NULL;
}

driver_error_t *can_rx(int32_t unit, uint32_t *msg_id, uint8_t *msg_type, uint8_t *data, uint8_t *len) {
	CAN_frame_t frame;

	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if (!setup) {
		return driver_error(CAN_DRIVER, CAN_ERR_IS_NOT_SETUP, NULL);
	}

	can_ll_rx(&frame);

	*msg_id = frame.MsgID;
	*msg_type = 0;
	*len = frame.FIR.B.DLC;
	memcpy(data, &frame.data, *len);

	return NULL;
}

driver_error_t *can_add_filter(int32_t unit, int32_t fromId, int32_t toId) {
	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if ((fromId < 0) || (toId < 0)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_FILTER, "must be >= 0");
	}

	if ((fromId > toId)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_FILTER, "from filter must be >= to filter");
	}

	if (!setup) {
		return driver_error(CAN_DRIVER, CAN_ERR_IS_NOT_SETUP, NULL);
	}

	// Check if there is some filter that match with the
	// desired filter
	uint8_t i;

	mtx_lock(&mtx);

	for(i=0;i < CAN_NUM_FILTERS;i++) {
		if ((fromId >= can_filter[i].fromID) && (toId <= can_filter[i].toID)) {
			mtx_unlock(&mtx);

			return NULL;
		}
	}

	// Add filter
	for(i=0;i < CAN_NUM_FILTERS;i++) {
		if ((can_filter[i].fromID  == -1) && (can_filter[i].toID == -1)) {
			can_filter[i].fromID = fromId;
			can_filter[i].toID = toId;
			filters++;
			break;
		}
	}

	if (i == CAN_NUM_FILTERS) {
		mtx_unlock(&mtx);

		return driver_error(CAN_DRIVER, CAN_ERR_NO_MORE_FILTERS_ALLOWED, NULL);
	}

	// Reset rx queue
	portDISABLE_INTERRUPTS();
	xQueueReset(CAN_cfg.rx_queue);
	portENABLE_INTERRUPTS();

	mtx_unlock(&mtx);

	return NULL;
}

driver_error_t *can_remove_filter(int32_t unit, int32_t fromId, int32_t toId) {
	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if ((fromId < 0) || (toId < 0)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_FILTER, "must be >= 0");
	}

	if ((fromId > toId)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_FILTER, "from filter must be >= to filter");
	}

	if (!setup) {
		return driver_error(CAN_DRIVER, CAN_ERR_IS_NOT_SETUP, NULL);
	}

	mtx_lock(&mtx);

	uint8_t i;
	for(i=0;i < CAN_NUM_FILTERS;i++) {
		if ((can_filter[i].fromID  > -1) && (can_filter[i].toID > -1)) {
			if ((can_filter[i].fromID  == fromId) && (can_filter[i].toID == toId)) {
				can_filter[i].fromID = -1;
				can_filter[i].toID = -1;
				filters--;
				break;
			}
		}
	}

	// Reset rx queue
	portDISABLE_INTERRUPTS();
	xQueueReset(CAN_cfg.rx_queue);
	portENABLE_INTERRUPTS();

	mtx_unlock(&mtx);

	return NULL;
}

driver_error_t *can_gateway_start(int32_t unit, uint16_t speed, int32_t port) {
	driver_error_t *error;

	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if ((error = can_setup(unit, speed))) {
		return error;
	}

	// Allocate space for configuration
	gw_config = calloc(1, sizeof(can_gw_config_t));
	if (!gw_config) {
		return driver_error(CAN_DRIVER, CAN_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	gw_config->port = port;
	gw_config->stop = 0;

	// Start main thread
	pthread_attr_t attr;

	pthread_attr_init(&attr);

	if (pthread_create(&gw_config->thread, &attr, gw_thread, NULL)) {
		free(gw_config);

		return driver_error(CAN_DRIVER, CAN_ERR_NOT_ENOUGH_MEMORY, "can't start main thread");
	}

	pthread_setname_np(gw_config->thread, "can gw");

	return NULL;
}

driver_error_t *can_gateway_stop(int32_t unit) {
	// Sanity checks
	if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
		return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
	}

	if (!gw_config) {
		return driver_error(CAN_DRIVER, CAN_ERR_GW_NOT_STARTED, NULL);
	}

	// Set stop flag
	gw_config->stop = 1;

	// Send an empty frame to the queue to unblock gw_thread_up
	CAN_frame_t frame;
	memset(&frame, 0, sizeof(frame));
	xQueueSend(CAN_cfg.rx_queue,&frame,0);

	// Close socket to unblock gw_thread
	close(gw_config->socket);

	pthread_join(gw_config->thread, NULL);

	free(gw_config);
	gw_config = NULL;

	// Reset queue
	mtx_lock(&mtx);
	portDISABLE_INTERRUPTS();
	xQueueReset(CAN_cfg.rx_queue);
	portENABLE_INTERRUPTS();
	mtx_unlock(&mtx);

	return NULL;
}

#endif

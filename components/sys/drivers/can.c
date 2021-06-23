/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2019, Jaume Olivé Petrus (jolive@whitecatboard.org)
 * Copyright (C) 2015 - 2019, Thomas E. Horner (whitecatboard.org@horner.it)
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

#include <driver/can.h>

#include <sys/driver.h>
#include <sys/syslog.h>
#include <sys/mutex.h>

#include <drivers/cpu.h>
#include <drivers/can.h>
#include <drivers/cpu.h>
#include <drivers/gpio.h>

#include <pthread.h>
#include <sys/panic.h>

static uint8_t setup = 0;

static can_timing_config_t t_config = CAN_TIMING_CONFIG_25KBITS();
/*
//Filter all other IDs except MSG_ID
static const can_filter_config_t f_config = {.acceptance_code = (MSG_ID << 21),
                                             .acceptance_mask = ~(CAN_STD_ID_MASK << 21),
                                             .single_filter = true};
*/
static can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
//Set to NO_ACK mode due to self testing with single module
static can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(CONFIG_LUA_RTOS_CAN_TX, CONFIG_LUA_RTOS_CAN_RX, CAN_MODE_NORMAL);
/*                                                                 {.mode = CAN_MODE_NO_ACK, .tx_io = CONFIG_LUA_RTOS_CAN_TX, .rx_io = CONFIG_LUA_RTOS_CAN_RX,       \
                                                                   .clkout_io = CAN_IO_UNUSED, .bus_off_io = CAN_IO_UNUSED,       \
                                                                   .tx_queue_len = 5, .rx_queue_len = 5,                          \
                                                                   .alerts_enabled = CAN_ALERT_NONE,  .clkout_divider = 0,        }
*/

// CAN gateway configuration
can_gw_config_t *gw_config = NULL;

// CAN filters
static uint8_t filters = 0;
static CAN_filter_t can_filter[CAN_NUM_FILTERS];

// Register driver and errors
DRIVER_REGISTER_BEGIN(CAN,can,0,NULL,NULL);
    DRIVER_REGISTER_ERROR(CAN, can, NotEnoughtMemory, "not enough memory", CAN_ERR_NOT_ENOUGH_MEMORY);
    DRIVER_REGISTER_ERROR(CAN, can, InvalidFrameLength, "invalid frame length", CAN_ERR_INVALID_FRAME_LENGTH);
    DRIVER_REGISTER_ERROR(CAN, can, InvalidUnit, "invalid unit", CAN_ERR_INVALID_UNIT);
    DRIVER_REGISTER_ERROR(CAN, can, NoMoreFiltersAllowed, "no more filters allowed", CAN_ERR_NO_MORE_FILTERS_ALLOWED);
    DRIVER_REGISTER_ERROR(CAN, can, InvalidFilter, "invalid filter", CAN_ERR_INVALID_FILTER);
    DRIVER_REGISTER_ERROR(CAN, can, NotSetup, "is not setup", CAN_ERR_IS_NOT_SETUP);
    DRIVER_REGISTER_ERROR(CAN, can, CannotStart, "can't start", CAN_ERR_CANT_START);
    DRIVER_REGISTER_ERROR(CAN, can, GatewayNotStarted, "gateway not started", CAN_ERR_GW_NOT_STARTED);
    DRIVER_REGISTER_ERROR(CAN, can, TransmitFail, "error transmitting message", CAN_ERR_TRANSMIT_FAIL);
    DRIVER_REGISTER_ERROR(CAN, can, InvalidArg, "invalid argument", CAN_ERR_INVALID_ARGUMENT);
    DRIVER_REGISTER_ERROR(CAN, can, InvalidState, "invalid state", CAN_ERR_INVALID_STATE);
    DRIVER_REGISTER_ERROR(CAN, can, NotSupport, "can API is not supported yet", CAN_ERR_NOT_SUPPORT);
    DRIVER_REGISTER_ERROR(CAN, can, TransmitTimeout, "timeout transmitting message", CAN_ERR_TRANSMIT_TIMEOUT);
DRIVER_REGISTER_END(CAN,can,0,NULL,NULL);

#define CAN_DRIVER driver_get_by_name("can")

driver_error_t *can_check_error(esp_err_t error) {
    if (error == ESP_OK) return NULL;

    switch (error) {
        case ESP_FAIL:                 return driver_error(CAN_DRIVER, CAN_ERR_TRANSMIT_FAIL,NULL);
        case ESP_ERR_NO_MEM:           return driver_error(CAN_DRIVER, CAN_ERR_NOT_ENOUGH_MEMORY,NULL);
        case ESP_ERR_INVALID_ARG:      return driver_error(CAN_DRIVER, CAN_ERR_INVALID_ARGUMENT,NULL);
        case ESP_ERR_NOT_SUPPORTED:    return driver_error(CAN_DRIVER, CAN_ERR_NOT_SUPPORT,NULL);
        case ESP_ERR_INVALID_STATE:    return driver_error(CAN_DRIVER, CAN_ERR_INVALID_STATE,NULL);
        case ESP_ERR_TIMEOUT:          return driver_error(CAN_DRIVER, CAN_ERR_TRANSMIT_TIMEOUT,NULL);

        default: {
            char *buffer;

            buffer = malloc(40);
            if (!buffer) {
                panic("not enough memory");
            }

            snprintf(buffer, 40, "missing can error case %d", error);

            return driver_error(CAN_DRIVER, CAN_ERR_CANT_START, buffer);
        }
    }

    return NULL;
}

/*
 * Helper functions
 */

#define MAX_BUS_ERROR 127
#define MAX_WAITMS_TX 100

void can_recovery() {
    can_status_info_t status_info;
    esp_err_t error = can_get_status_info(&status_info);
    if (error == ESP_OK && status_info.state==CAN_STATE_RUNNING && status_info.bus_error_count<MAX_BUS_ERROR) {
        ; //do nothing
    } else {
        can_start();
        can_initiate_recovery();
    }
}

static driver_error_t *can_ll_tx(can_message_t *frame) {
    driver_error_t *error;
    if ((error = can_check_error(can_transmit(frame, pdMS_TO_TICKS(MAX_WAITMS_TX))))) {
        can_recovery();
        return error;
    }
    return 0;
}

static driver_error_t *can_ll_rx(can_message_t *frame, uint32_t timeout) {
    // Read next frame
    // Check filter
    uint8_t i;
    uint8_t pass = 0;
    driver_error_t *error;

    if (timeout != portMAX_DELAY) {
        timeout = timeout / portTICK_PERIOD_MS;
    }

    while (!pass) {
        if ((error = can_check_error(can_receive(frame, timeout)))) {
            can_recovery();
            return error;
        }
        pass = 1;
        if (filters > 0) {
            CAN_FIR_t FIR = (CAN_FIR_t)frame->flags;
            for(i=0;((i < CAN_NUM_FILTERS) && (!pass));i++) {
                if ((can_filter[i].fromID >= 0) && (can_filter[i].toID >= 0) && (FIR.B.DLC > 0) && (FIR.B.DLC < 9)) {
                    pass = ((frame->identifier >= can_filter[i].fromID) && (frame->identifier <= can_filter[i].toID));
                }
            }
        } else {
            pass = 1;
        }
    }

    return 0;
}

static void *gw_thread_up(void *arg) {
    can_message_t frame;
    struct can_frame packet;

    // Set a timeout for send
    struct timeval tout;
    tout.tv_sec = 4;
    tout.tv_usec = 0;
    setsockopt(gw_config->client, SOL_SOCKET, SO_SNDTIMEO, &tout, sizeof(tout));

    while(!gw_config->stop) {
        // Wait for a CAN packet
        can_ll_rx(&frame, portMAX_DELAY);
        CAN_FIR_t FIR = (CAN_FIR_t)frame.flags;
        if (FIR.B.DLC != 0) {
            // Fill packet
            memset(&packet, 0, sizeof(packet));

            packet.can_id = (FIR.B.FF << 31) | (FIR.B.RTR << 30) | frame.identifier;
            packet.can_dlc = FIR.B.DLC;

            memcpy(packet.data, frame.data, sizeof(frame.data));

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
    can_message_t frame;

    while(!gw_config->stop) {
        if (read(gw_config->client, &packet, sizeof(packet)) == sizeof(packet)) {
            // Fill frame
            frame.identifier = (packet.can_id & 0b11111111111111111111111111111);
            CAN_FIR_t FIR;
            memset(&FIR, 0, sizeof(CAN_FIR_t));
            FIR.B.FF = (packet.can_id & 0b10000000000000000000000000000000?1:0);
            FIR.B.RTR = (packet.can_id & 0b01000000000000000000000000000000?1:0);
            FIR.B.DLC = packet.can_dlc;
            memcpy(&frame.flags, &FIR, sizeof(CAN_FIR_t));

            memcpy(frame.data, packet.data, sizeof(packet.data));

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

            pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
            if (pthread_create(&thread_up, &attr, gw_thread_up, NULL)) {
                syslog(LOG_ERR, "can%d gateway: can't start up thread", gw_config->unit);
                return NULL;
            }

            pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
            if (pthread_create(&thread_down, &attr, gw_thread_down, NULL)) {
                syslog(LOG_ERR, "can%d gateway: can't start down thread", gw_config->unit);
                return NULL;
            }

            pthread_setname_np(thread_up, "can_gw_up");
            pthread_setname_np(thread_down, "can_gw_down");

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

driver_error_t *can_setup(int32_t unit, uint32_t speed, uint16_t rx_size) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unit_lock_error_t *lock_error = NULL;
#endif

    // Sanity checks
    if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
        return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
    }

    if (!setup) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
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
#endif
    }

    g_config.rx_queue_len = rx_size;
    g_config.tx_queue_len = rx_size;
    switch(speed) {
        case 1000: {
            can_timing_config_t set_config = CAN_TIMING_CONFIG_1MBITS();
            memcpy(&t_config, &set_config, sizeof(set_config));
            }
            break;
        case 800: {
            can_timing_config_t set_config = CAN_TIMING_CONFIG_800KBITS();
            memcpy(&t_config, &set_config, sizeof(set_config));
            }
            break;
        case 500: {
            can_timing_config_t set_config = CAN_TIMING_CONFIG_500KBITS();
            memcpy(&t_config, &set_config, sizeof(set_config));
            }
            break;
        case 250: {
            can_timing_config_t set_config = CAN_TIMING_CONFIG_250KBITS();
            memcpy(&t_config, &set_config, sizeof(set_config));
            }
            break;
        case 125: {
            can_timing_config_t set_config = CAN_TIMING_CONFIG_125KBITS();
            memcpy(&t_config, &set_config, sizeof(set_config));
            }
            break;
        case 100: {
            can_timing_config_t set_config = CAN_TIMING_CONFIG_100KBITS();
            memcpy(&t_config, &set_config, sizeof(set_config));
            }
            break;
        case 50: {
            can_timing_config_t set_config = CAN_TIMING_CONFIG_50KBITS();
            memcpy(&t_config, &set_config, sizeof(set_config));
            }
            break;
        default: {
            can_timing_config_t set_config = CAN_TIMING_CONFIG_25KBITS();
            memcpy(&t_config, &set_config, sizeof(set_config));
            }
    }

    // Start CAN module
    driver_error_t *error;
    if ((error = can_check_error(can_driver_install(&g_config, &t_config, &f_config)))) {
        return error;
    }
    if ((error = can_check_error(can_start()))) {
        return error;
    }

    if (!setup) {
        syslog(LOG_INFO, "can%d at pins tx=%s%d, rx=%s%d", 0,
            gpio_portname(CONFIG_LUA_RTOS_CAN_TX), gpio_name(CONFIG_LUA_RTOS_CAN_TX),
            gpio_portname(CONFIG_LUA_RTOS_CAN_RX), gpio_name(CONFIG_LUA_RTOS_CAN_RX)
        );
    }

    setup = 1;
    return NULL;
}

driver_error_t *can_tx(int32_t unit, uint32_t msg_id, uint8_t msg_type, uint8_t *data, uint8_t len) {
    can_message_t frame;

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
    frame.identifier = msg_id;
    frame.data_length_code = len;
    frame.flags = (msg_type == 1 ? CAN_MSG_FLAG_EXTD:CAN_MSG_FLAG_NONE);
    memcpy(&frame.data, data, len);

    return can_ll_tx(&frame);
}

driver_error_t *can_rx(int32_t unit, uint32_t *msg_id, uint8_t *msg_type, uint8_t *data, uint8_t *len, uint32_t timeout) {
    can_message_t frame;

    // Sanity checks
    if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
        return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
    }

    if (!setup) {
        return driver_error(CAN_DRIVER, CAN_ERR_IS_NOT_SETUP, NULL);
    }

    driver_error_t *error = can_ll_rx(&frame, timeout);
    if (!error) {
        *msg_id = frame.identifier;
        *msg_type = (frame.flags & CAN_MSG_FLAG_EXTD ? 1 : 0);
        *len = frame.data_length_code;
        memcpy(data, &frame.data, *len);
    }
    return error;
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

    for(i=0;i < CAN_NUM_FILTERS;i++) {
        if ((fromId >= can_filter[i].fromID) && (toId <= can_filter[i].toID)) {
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
        return driver_error(CAN_DRIVER, CAN_ERR_NO_MORE_FILTERS_ALLOWED, NULL);
    }

	// Reset rx queue
	driver_error_t *error;
    if ((error = can_check_error(can_stop()))) {
        return error;
    }
    if ((error = can_check_error(can_start()))) {
        return error;
    }

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
	driver_error_t *error;
    if ((error = can_check_error(can_stop()))) {
        return error;
    }
    if ((error = can_check_error(can_start()))) {
        return error;
    }

    return NULL;
}

driver_error_t *can_gateway_start(int32_t unit, uint32_t speed, int32_t port) {
    driver_error_t *error;

    // Sanity checks
    if ((unit < CPU_FIRST_CAN) || (unit > CPU_LAST_CAN)) {
        return driver_error(CAN_DRIVER, CAN_ERR_INVALID_UNIT, NULL);
    }

    if ((error = can_setup(unit, speed, 100))) {
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

    pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (pthread_create(&gw_config->thread, &attr, gw_thread, NULL)) {
        free(gw_config);

        return driver_error(CAN_DRIVER, CAN_ERR_NOT_ENOUGH_MEMORY, "can't start main thread");
    }

    pthread_setname_np(gw_config->thread, "can_gw");

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
    can_message_t frame;
    memset(&frame, 0, sizeof(frame));
    driver_error_t *error;
    if ((error = can_ll_tx(&frame))) {
        return error;
    }

    // Close socket to unblock gw_thread
    close(gw_config->socket);

    pthread_join(gw_config->thread, NULL);

    free(gw_config);
    gw_config = NULL;

	// Reset rx queue
    if ((error = can_check_error(can_stop()))) {
        return error;
    }
    if ((error = can_check_error(can_start()))) {
        return error;
    }

    return NULL;
}

#endif

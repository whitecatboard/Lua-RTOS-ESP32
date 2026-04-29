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
 * Lua RTOS, LoRaWAN driver for Semtech Stack
 *
 */

#ifndef LORA_H
#define LORA_H

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <stdint.h>
#include <sys/driver.h>

#define LORA_DRIVER driver_get_by_name("lora")

// LoRa errors
#define LORA_ERR_NOT_SETUP (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) | 0)
#define LORA_ERR_NO_MEM (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) | 1)
#define LORA_ERR_UNEXPECTED_RESPONSE (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) | 2)
#define LORA_ERR_INVALID_ARGUMENT (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) | 3)
#define LORA_ERR_JOIN_ERROR (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) | 4)
#define LORA_ERR_NOT_JOINED (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) | 5)
#define LORA_ERR_MAC_ERROR (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) | 6)
#define LORA_ERR_TX_ERROR_NACK (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) | 7)
#define LORA_ERR_TX_ERROR (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) | 8)
#define LORA_ERR_TIMEOUT (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) | 9)

extern const int lora_errors;
extern const int lora_error_map;

// Semtech stack configuration
#if CONFIG_LUA_RTOS_LORA_BAND_EU868
#define ACTIVE_REGION LORAMAC_REGION_EU868
#endif

#define LORAWAN_DEFAULT_CLASS CLASS_A
#define LORAWAN_ADR_STATE LORAMAC_HANDLER_ADR_OFF
#define LORAWAN_DEFAULT_DATARATE DR_3
#define LORAWAN_DEFAULT_CONFIRMED_MSG_STATE LORAMAC_HANDLER_UNCONFIRMED_MSG
#define LORAWAN_APP_DATA_BUFFER_MAX_SIZE 242
#define LORAWAN_MTU_SIZE 255
#define LORAWAN_DUTYCYCLE_ON true

// LoRa MAC set commands
#define LORA_MAC_SET_DEVADDR 0
#define LORA_MAC_SET_DEVEUI 1
#define LORA_MAC_SET_JOINEUI 2
#define LORA_MAC_SET_APPKEY 3
#define LORA_MAC_SET_NWKKEY 4
#define LORA_MAC_SET_DR 5
#define LORA_MAC_SET_ADR 6
#define LORA_MAC_SET_NB_TRANS 7
#define LORA_MAC_SET_DEVICE_CLASS 8

// LoRa MAC get commands
#define LORA_MAC_GET_DEVADDR 20
#define LORA_MAC_GET_DEVEUI 21
#define LORA_MAC_GET_JOINEUI 22
#define LORA_MAC_GET_DR 23
#define LORA_MAC_GET_ADR 24
#define LORA_MAC_GET_LINKCHK 25
#define LORA_MAC_GET_NB_TRANS 26
#define LORA_MAC_GET_DEVICE_CLASS 27

// LoRa MAC sizes for hex strings
#define LORA_MAC_MAX_PAYLOAD_SIZE (LORAWAN_APP_DATA_BUFFER_MAX_SIZE << 1)
#define LORA_MAC_DEVADDR_SIZE 8
#define LORA_MAC_EUI_SIZE 16
#define LORA_MAC_KEY_SIZE 32

typedef struct {
  uint32_t frequency;
  uint32_t time_on_air;
  uint32_t time_off_air;
  uint8_t dr;
} lora_tx_ret_t;

typedef struct {
  uint8_t demod_margin;
  uint8_t nb_gateways;
} lora_link_check_req_ret_t;

typedef struct {
  uint8_t port;
  int8_t dr;
  int8_t rssi;
  int8_t snr;
  uint32_t cnt;
  int8_t slot;  
  uint8_t size;
  uint8_t *payload[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];
} lora_downlink_t;

typedef void(lora_rx)(int port, char *payload);

driver_error_t *lora_setup(int band);
driver_error_t *lora_mac_set(const char command, const uint8_t *value);
driver_error_t *lora_mac_get(const char command, uint8_t *value);
driver_error_t *lora_join();
driver_error_t *lora_tx(int regular, int cnf, int port, const uint8_t *buffer, uint16_t size, lora_tx_ret_t *tx_ret);
driver_error_t *lora_device_time_req(void);
driver_error_t *lora_link_check_req(lora_link_check_req_ret_t *link_check_req_ret);
QueueHandle_t lora_get_rx_queue_h(void);

#endif /* LORA_H */

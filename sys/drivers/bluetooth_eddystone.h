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
 * Lua RTOS, EDDYSTONE BEACON SERVICE
 *
 */

#ifndef BT_EDDYSTONE_H_
#define BT_EDDYSTONE_H_

#include "sdkconfig.h"

#if CONFIG_BT_ENABLED

#include "bluetooth.h"

#include <stdint.h>

#include <sys/driver.h>

typedef enum {
	EddystoneUID = 0,
	EddystoneURL = 1
} bt_eddystone_type_t;

typedef int8_t  bt_eddystone_tx_power_t;
typedef uint8_t bt_eddystone_namespace_t[10];
typedef uint8_t bt_eddystone_instance_t[6];

typedef struct {
	uint8_t tx_power;
	uint8_t name_space[10];
	uint8_t instance[6];
} bt_eddystone_uid_t;

typedef enum {
	HTTP_WWW = 0,  ///< http://www.
	HTTPS_WWW = 1, ///< https://www.
	HTTP = 2,	   ///< http://
	HTTPS = 3	   ///< https://
} bt_eddystone_url_scheme_prefix_t;

typedef enum {
	DOT_COM_P = 0x00,  ///< .com/
	DOT_ORG_P = 0x01,  ///< .org/
	DOT_EDU_P = 0x02,  ///< .edu/
	DOT_NET_P = 0x03,  ///< .net/
	DOT_INFO_P = 0x04, ///< .info/
	DOT_BIZ_P = 0x05,  ///< .biz/
	DOT_GOV_P = 0x06,  ///< .gov/
	DOT_COM_S =	0x07,  ///< .com
	DOT_ORG_S = 0x08,  ///< .org
	DOT_EDU_S =	0x09,  ///< .edu
	DOT_NET_S = 0x0a,  ///< .net
	DOT_INFO_S = 0x0b, ///< .info
	DOT_BIZ_S = 0x0c,  ///< .biz
	DOT_GOV_S = 0x0d,  ///< .gov
} bt_eddystone_url_http_url_encoding_t;

typedef struct {
	uint8_t tx_power;
	bt_eddystone_url_scheme_prefix_t prefix;
	uint8_t encoded_url_len;
	uint8_t encoded_url[17];
} bt_eddystone_url_t;

typedef struct {
	uint8_t started;
	bt_eddystone_type_t type;
	bte_advertise_params_t advParams;

	union {
		bt_eddystone_uid_t uid;
		bt_eddystone_url_t url;
	} data;
} bt_eddystone_t;

/**
 * @brief Add an eddystone uid frame to the eddystone advertising manager. The
 *        uid is not advertised until user call the bt_eddystone_start function.
 *
 * @param address Broadcast address.
 * @param tx_power TX power level.
 * @param namespace Namespace.
 * @param instance Instance.
 * @param beacon_h Pointer to a variable to hold the beacon handle.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 BT_ERR_NOT_ENOUGH_MEMORY
 */
driver_error_t *bt_add_eddystone_uid(
	bt_adress_t address,
	bt_eddystone_tx_power_t tx_power,
	bt_eddystone_namespace_t namespace,
	bt_eddystone_instance_t instance,
	int *beacon_h);

/**
 * @brief Remove a beacon from the eddystone advertising manager.
 *
 * @param beacon_h Beacon handle to remove.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 BT_ERR_INVALID_BEACON
 */
driver_error_t *bt_eddystone_remove(int beacon_h);

/**
 * @brief Start beacon advertising.
 *
 * @param beacon_h Beacon handle to start.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 BT_ERR_INVALID_BEACON
 */
driver_error_t *bt_eddystone_start(int beacon_h);

/**
 * @brief Stop beacon advertising.
 *
 * @param beacon_h Beacon handle to start.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 BT_ERR_INVALID_BEACON
 */
driver_error_t *bt_eddystone_stop(int beacon_h);

driver_error_t *bt_add_eddystone_url(
	bt_adress_t address,
	bt_eddystone_tx_power_t tx_power,
	const char *url,
	int *beacon_h);

/**
 * @brief Decode an eddystone beacon
 *
 * @param data Advertise frame
 * @param len Advertise's frame length
 * @param decoded Decoded beacon
 *
 */
void bt_eddystone_decode(uint8_t *data, uint8_t len, bt_adv_frame_t *decoded);

#endif /* BT_EDDYSTONE_H_ */

#endif

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
 * Lua RTOS, HCI API
 *
 */

#include "sdkconfig.h"

#if CONFIG_BT_ENABLED

#include "bluetooth.h"

#include <stdint.h>
#include <string.h>

#include <sys/delay.h>

driver_error_t *HCI_Reset() {
	uint8_t buf[HCI_H4_CMD_PREAMBLE_SIZE];
	uint8_t *pbuf;

	// Build hci command to reset the controller
	pbuf = buf;

	UINT8_TO_STREAM(pbuf, H4_TYPE_COMMAND);
	UINT16_TO_STREAM(pbuf,HCI_RESET);
	UINT8_TO_STREAM(pbuf, 0);

	// Send command to controller
	while (!esp_vhci_host_check_send_available())
		delay(100);
	esp_vhci_host_send_packet(buf, sizeof(buf));

	return NULL;
}

driver_error_t *HCI_Set_Event_Mask(uint8_t *mask) {
	// Send HCI command to set event mask
	uint8_t buf[HCI_H4_CMD_PREAMBLE_SIZE + HCI_SET_EVENT_MASK_SIZE];
	uint8_t *pbuf;

	pbuf = buf;

	UINT8_TO_STREAM(pbuf, H4_TYPE_COMMAND);
	UINT16_TO_STREAM(pbuf,HCI_SET_EVENT_MASK);
	UINT8_TO_STREAM(pbuf, HCI_SET_EVENT_MASK_SIZE);
	UINT8_TO_STREAM(pbuf, mask[0]);
	UINT8_TO_STREAM(pbuf, mask[1]);
	UINT8_TO_STREAM(pbuf, mask[2]);
	UINT8_TO_STREAM(pbuf, mask[3]);
	UINT8_TO_STREAM(pbuf, mask[4]);
	UINT8_TO_STREAM(pbuf, mask[5]);
	UINT8_TO_STREAM(pbuf, mask[6]);
	UINT8_TO_STREAM(pbuf, mask[7]);

	while (!esp_vhci_host_check_send_available())
		delay(100);
	esp_vhci_host_send_packet(buf, sizeof(buf));

	return NULL;
}

driver_error_t *HCI_LE_Set_Event_Mask(uint8_t *mask) {
	// Send HCI command to set event mask
	uint8_t buf[HCI_H4_CMD_PREAMBLE_SIZE + HCI_BLE_SET_EVENT_MASK_SIZE];
	uint8_t *pbuf;

	pbuf = buf;

	UINT8_TO_STREAM(pbuf, H4_TYPE_COMMAND);
	UINT16_TO_STREAM(pbuf,HCI_BLE_SET_EVENT_MASK);
	UINT8_TO_STREAM(pbuf, HCI_BLE_SET_EVENT_MASK_SIZE);
	UINT8_TO_STREAM(pbuf, mask[0]);
	UINT8_TO_STREAM(pbuf, mask[1]);
	UINT8_TO_STREAM(pbuf, mask[2]);
	UINT8_TO_STREAM(pbuf, mask[3]);
	UINT8_TO_STREAM(pbuf, mask[4]);
	UINT8_TO_STREAM(pbuf, mask[5]);
	UINT8_TO_STREAM(pbuf, mask[6]);
	UINT8_TO_STREAM(pbuf, mask[7]);

	while (!esp_vhci_host_check_send_available())
		delay(100);
	esp_vhci_host_send_packet(buf, sizeof(buf));

	return NULL;
}

driver_error_t *HCI_LE_Set_Advertising_Parameters(bte_advertise_params_t params) {
	// Sanity checks
	if ((params.interval_min < 0x0020) || (params.interval_min > 0x4000)) {
		return driver_error(BT_DRIVER, BT_ERR_INVALID_ARGUMENT, "min interval must be between 0x0020 and 0x4000");
	}

	if ((params.interval_max < 0x0020) || (params.interval_max > 0x4000)) {
		return driver_error(BT_DRIVER, BT_ERR_INVALID_ARGUMENT, "max interval must be between 0x0020 and 0x4000");
	}

	if (params.interval_min > params.interval_max) {
		return driver_error(BT_DRIVER, BT_ERR_INVALID_ARGUMENT, "min interval less than or equal to max interal");
	}

	// Send HCI command to set the advertise params
	uint8_t buf[HCI_H4_CMD_PREAMBLE_SIZE + HCI_BLE_SET_ADV_PARAMS_SIZE];
	uint8_t *pbuf;

	pbuf = buf;

	UINT8_TO_STREAM(pbuf,  H4_TYPE_COMMAND);
	UINT16_TO_STREAM(pbuf, HCI_BLE_SET_ADV_PARAMS);
	UINT8_TO_STREAM(pbuf,  HCI_BLE_SET_ADV_PARAMS_SIZE);
	UINT16_TO_STREAM(pbuf, (uint16_t )(params.interval_min));
	UINT16_TO_STREAM(pbuf, (uint16_t )(params.interval_max));
	UINT8_TO_STREAM(pbuf,  params.type);
	UINT8_TO_STREAM(pbuf,  params.own_address_type);
	UINT8_TO_STREAM(pbuf,  params.peer_address_type);
	BDADDR_TO_STREAM(pbuf, params.peer_address);
	UINT8_TO_STREAM(pbuf,  params.chann_map);
	UINT8_TO_STREAM(pbuf,  params.filter_policy);

    while (!esp_vhci_host_check_send_available()) delay(100);
    esp_vhci_host_send_packet(buf, sizeof(buf));

    return NULL;
}

driver_error_t *HCI_LE_Set_Advertising_Data(uint8_t *adv_data, uint16_t adv_data_len) {
	// Sanity checks
	if (!adv_data) {
		return driver_error(BT_DRIVER, BT_ERR_INVALID_ARGUMENT, "missing advertise data");
	}

	if (adv_data_len == 0) {
		return driver_error(BT_DRIVER, BT_ERR_INVALID_ARGUMENT, "missing advertise data");
	}

	// Send HCI command to set advertise data
	uint8_t buf[HCI_H4_CMD_PREAMBLE_SIZE + HCI_BLE_SET_ADV_DATA_SIZE + 1];
	uint8_t *pbuf;

	pbuf = buf;

    UINT8_TO_STREAM  (pbuf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM (pbuf, HCI_BLE_SET_ADV_DATA);
    UINT8_TO_STREAM  (pbuf, HCI_BLE_SET_ADV_DATA_SIZE + 1);

    memset(pbuf, 0, HCI_BLE_SET_ADV_DATA_SIZE);

    if (adv_data != NULL && adv_data_len > 0) {
        if (adv_data_len > HCI_BLE_SET_ADV_DATA_SIZE) {
        	adv_data_len = HCI_BLE_SET_ADV_DATA_SIZE;
        }

        UINT8_TO_STREAM (pbuf, adv_data_len);
        ARRAY_TO_STREAM (pbuf, adv_data, adv_data_len);
    }

    // Send command to controller
    while (!esp_vhci_host_check_send_available()) delay(100);
    esp_vhci_host_send_packet(buf, sizeof(buf));

    return NULL;
}

driver_error_t *HCI_LE_Set_Advertise_Enable(uint8_t enable) {
	uint8_t buf[HCI_H4_CMD_PREAMBLE_SIZE + HCI_BLE_SET_ADV_ENABLE_SIZE];
	uint8_t *pbuf;

    // Send HCI command to enable advertising
    pbuf = buf;

	UINT8_TO_STREAM(pbuf,  H4_TYPE_COMMAND);
	UINT16_TO_STREAM(pbuf, HCI_BLE_SET_ADV_ENABLE);
	UINT8_TO_STREAM(pbuf,  HCI_BLE_SET_ADV_ENABLE_SIZE);
	UINT8_TO_STREAM(pbuf, enable);

    // Send command to controller
    while (!esp_vhci_host_check_send_available()) delay(100);
    esp_vhci_host_send_packet(buf, sizeof(buf));

    return NULL;
}

#endif

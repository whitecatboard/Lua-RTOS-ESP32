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
 * Lua RTOS, BT driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_BT_ENABLED

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "bluetooth.h"
#include "esp_bt.h"

#include <stdint.h>
#include <string.h>

#include <sys/delay.h>
#include <sys/driver.h>
#include <sys/syslog.h>

DRIVER_REGISTER_BEGIN(BT,bt,0,NULL,NULL);
	DRIVER_REGISTER_ERROR(BT, bt, CannotSetup, "can't setup", BT_ERR_CANT_INIT);
	DRIVER_REGISTER_ERROR(BT, bt, InvalidMode, "invalid mode", BT_ERR_INVALID_MODE);
	DRIVER_REGISTER_ERROR(BT, bt, NotSetup, "is not setup", BT_ERR_IS_NOT_SETUP);
	DRIVER_REGISTER_ERROR(BT, bt, NotEnoughtMemory, "not enough memory", BT_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(BT, bt, InvalidArgument, "invalid argument", BT_ERR_INVALID_ARGUMENT);
	DRIVER_REGISTER_ERROR(BT, bt, InvalidBeacon, "invalid beacon", BT_ERR_INVALID_BEACON);
	DRIVER_REGISTER_ERROR(BT, bt, CannotStartScan, "can't start scanning", BT_ERR_CANT_START_SCAN);
	DRIVER_REGISTER_ERROR(BT, bt, CannotStopScan, "can't stop scanning", BT_ERR_CANT_STOP_SCAN);
	DRIVER_REGISTER_ERROR(BT, bt, CannotStartAdv, "can't start advertising", BT_ERR_CANT_START_ADV);
	DRIVER_REGISTER_ERROR(BT, bt, CannotStopAdv, "can't start advertising", BT_ERR_CANT_STOP_ADV);
	DRIVER_REGISTER_ERROR(BT, bt, InvalidTxPower, "invalid tx power", BT_ERR_INVALID_TX_POWER);
	DRIVER_REGISTER_ERROR(BT, bt, AdvTooLong, "advertising data must be less then 31 bytes", BT_ERR_ADVDATA_TOO_LONG);
DRIVER_REGISTER_END(BT,bt,0,NULL,NULL);

#define evBT_SCAN_START_COMPLETE ( 1 << 0 )
#define evBT_SCAN_START_ERROR    ( 1 << 1 )
#define evBT_SCAN_STOP_COMPLETE  ( 1 << 2 )
#define evBT_SCAN_STOP_ERROR     ( 1 << 3 )
#define evBT_ADV_START_COMPLETE  ( 1 << 4 )
#define evBT_ADV_START_ERROR     ( 1 << 5 )
#define evBT_ADV_STOP_COMPLETE   ( 1 << 6 )
#define evBT_ADV_STOP_ERROR      ( 1 << 7 )

#define GAP_CB_CHECK(s, ok_ev, error_ev) \
if(s != ESP_BT_STATUS_SUCCESS) { \
	xEventGroupSetBits(bt_event, error_ev); \
} else { \
	xEventGroupSetBits(bt_event, ok_ev); \
	break; \
}

// Is BT setup?
static uint8_t setup = 0;

// Event group to sync some driver functions with the event handler
EventGroupHandle_t bt_event;

// Queue and task to get data generated in the event handler
static xQueueHandle queue = NULL;
static TaskHandle_t task = NULL;

bt_scan_callback_t callback;
static int callback_id;

/*
 * Helper functions
 */
static void bt_task(void *arg) {
	bt_adv_frame_t data;

	memset(&data,0,sizeof(bt_adv_frame_t));

    for(;;) {
        xQueueReceive(queue, &data, portMAX_DELAY);
        callback(callback_id, &data);
    }
}

static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
	switch (event) {
		case ESP_GAP_BLE_ADV_START_COMPLETE_EVT: {
			GAP_CB_CHECK(param->adv_start_cmpl.status, evBT_ADV_START_COMPLETE, evBT_ADV_START_ERROR);
			break;
		}

		case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT: {
			GAP_CB_CHECK(param->adv_stop_cmpl.status, evBT_ADV_STOP_COMPLETE, evBT_ADV_STOP_ERROR);
			break;
		}

		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
			uint32_t duration = 0;
			esp_ble_gap_start_scanning(duration);
			break;
		}

		case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
			GAP_CB_CHECK(param->scan_start_cmpl.status, evBT_SCAN_START_COMPLETE, evBT_SCAN_START_ERROR);
			break;
		}

	case ESP_GAP_BLE_SCAN_RESULT_EVT: {
		esp_ble_gap_cb_param_t* scan_result = param;

		switch (scan_result->scan_rst.search_evt) {
			case ESP_GAP_SEARCH_INQ_RES_EVT: {
				bt_adv_frame_t frame;

				// Get RSSI
				frame.rssi = scan_result->scan_rst.rssi;

				// Get raw data
				frame.len = scan_result->scan_rst.adv_data_len;
				memcpy(frame.raw, scan_result->scan_rst.ble_adv, frame.len);

				bt_eddystone_decode(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len, &frame);
				xQueueSend(queue, &frame, 0);

				break;
			}

			default:
				break;
		}

		break;
	}

	case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
		GAP_CB_CHECK(param->scan_stop_cmpl.status, evBT_SCAN_STOP_COMPLETE, evBT_SCAN_STOP_ERROR);
		break;
	}

	default:
		break;
	}
}

/*
 * Operation functions
 */

driver_error_t *bt_setup(bt_mode_t mode) {
	// Sanity checks
	if (setup) {
		return NULL;
	}

	if (mode > Dual) {
		return driver_error(BT_DRIVER, BT_ERR_INVALID_MODE, NULL);
	}

	// Create an event group sync some driver functions with the event handler
	bt_event = xEventGroupCreate();

	queue = xQueueCreate(10, sizeof(bt_adv_frame_t));
	if (!queue) {
		return driver_error(BT_DRIVER, BT_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	BaseType_t xReturn = xTaskCreatePinnedToCore(bt_task, "bt", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &task, xPortGetCoreID());
	if (xReturn != pdPASS) {
		return driver_error(BT_DRIVER, BT_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Initialize BT controller
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	esp_bt_controller_init(&bt_cfg);

	// Enable BT controller
	if (esp_bt_controller_enable(mode) != ESP_OK) {
		return driver_error(BT_DRIVER, BT_ERR_CANT_INIT, NULL);
	}

    esp_bluedroid_init();
    esp_bluedroid_enable();
	esp_ble_gap_register_callback(&gap_cb);

	setup = 1;

	return NULL;
}

driver_error_t *bt_reset() {
	driver_error_t *error;

	// Sanity checks
	if (!setup) {
		return driver_error(BT_DRIVER, BT_ERR_IS_NOT_SETUP, NULL);
	}

	if ((error = HCI_Reset())) {
		return error;
	}

	return NULL;
}

driver_error_t *bt_adv_start(bte_advertise_params_t adv_params, uint8_t *adv_data, uint16_t adv_data_len) {
	// Sanity checks
	if (!setup) {
		return driver_error(BT_DRIVER, BT_ERR_IS_NOT_SETUP, NULL);
	}

	// Set advertising data
	esp_ble_gap_config_adv_data_raw(adv_data, adv_data_len);

	// Start advertising
	esp_ble_adv_params_t params;

	params.adv_int_min = adv_params.interval_min;
	params.adv_int_max = adv_params.interval_max;
	params.adv_type = adv_params.type;
	params.own_addr_type = adv_params.own_address_type;
	memcpy(params.peer_addr,adv_params.peer_address, sizeof(params.peer_addr));
	params.peer_addr_type = adv_params.peer_address_type;
	params.channel_map = adv_params.chann_map;
	params.adv_filter_policy = adv_params.filter_policy;

	esp_ble_gap_start_advertising(&params);

	// Wait for advertising start completion
	EventBits_t uxBits = xEventGroupWaitBits(bt_event, evBT_ADV_START_COMPLETE | evBT_ADV_START_ERROR, pdTRUE, pdFALSE, portMAX_DELAY);
	if (uxBits & (evBT_ADV_START_COMPLETE)) {
		return NULL;
	} else if (uxBits & (evBT_ADV_START_ERROR)) {
		return driver_error(BT_DRIVER, BT_ERR_CANT_START_ADV, NULL);
	}

	return NULL;
}

driver_error_t *bt_adv_stop() {
	// Sanity checks
	if (!setup) {
		return driver_error(BT_DRIVER, BT_ERR_IS_NOT_SETUP, NULL);
	}

	// Stop advertising
	esp_ble_gap_stop_advertising();

	// Wait for advertising stop completion
	EventBits_t uxBits = xEventGroupWaitBits(bt_event, evBT_ADV_STOP_COMPLETE | evBT_ADV_STOP_ERROR, pdTRUE, pdFALSE, portMAX_DELAY);
	if (uxBits & (evBT_ADV_STOP_COMPLETE)) {
		return NULL;
	} else if (uxBits & (evBT_ADV_STOP_ERROR)) {
		return driver_error(BT_DRIVER, BT_ERR_CANT_STOP_ADV, NULL);
	}

	return NULL;
}

driver_error_t *bt_scan_start(bt_scan_callback_t cb, int cb_id) {
	// Sanity checks
	if (!setup) {
		return driver_error(BT_DRIVER, BT_ERR_IS_NOT_SETUP, NULL);
	}

	// Store callback data
	callback = cb;
	callback_id = cb_id;

	// Set scan parameters
	esp_ble_scan_params_t *scan_params = calloc(1, sizeof(esp_ble_scan_params_t));
	if (!scan_params) {
		return driver_error(BT_DRIVER, BT_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	scan_params->scan_type           = BLE_SCAN_TYPE_ACTIVE;
	scan_params->own_addr_type       = BLE_ADDR_TYPE_PUBLIC;
	scan_params->scan_filter_policy  = BLE_SCAN_FILTER_ALLOW_ALL;
	scan_params->scan_interval       = 0x50;
	scan_params->scan_window         = 0x3;

	esp_ble_gap_set_scan_params(scan_params);

	// Wait for scan start completion
	EventBits_t uxBits = xEventGroupWaitBits(bt_event, evBT_SCAN_START_COMPLETE | evBT_SCAN_START_ERROR, pdTRUE, pdFALSE, portMAX_DELAY);
	if (uxBits & (evBT_SCAN_START_COMPLETE)) {
	} else if (uxBits & (evBT_SCAN_START_ERROR)) {
		free(scan_params);
		return driver_error(BT_DRIVER, BT_ERR_CANT_START_SCAN, NULL);
	}

	free(scan_params);
	return NULL;
}

driver_error_t *bt_scan_stop() {
	// Sanity checks
	if (!setup) {
		return driver_error(BT_DRIVER, BT_ERR_IS_NOT_SETUP, NULL);
	}

	// Stop scan
	esp_ble_gap_stop_scanning();

	// Wait for scan stop completion
	EventBits_t uxBits = xEventGroupWaitBits(bt_event, evBT_SCAN_STOP_COMPLETE | evBT_SCAN_STOP_ERROR, pdTRUE, pdFALSE, portMAX_DELAY);
	if (uxBits & (evBT_SCAN_STOP_COMPLETE)) {
		return NULL;
	} else if (uxBits & (evBT_SCAN_STOP_ERROR)) {
		return driver_error(BT_DRIVER, BT_ERR_CANT_STOP_SCAN, NULL);
	}

	return NULL;
}

#endif

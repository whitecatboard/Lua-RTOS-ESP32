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
 * Lua RTOS, BT driver
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "sdkconfig.h"

#include "bluetooth.h"
#include "esp_bt.h"

#include <stdint.h>
#include <string.h>

#include <sys/delay.h>
#include <sys/driver.h>
#include <sys/syslog.h>

DRIVER_REGISTER_BEGIN(BT,bt,NULL,NULL,NULL);
	DRIVER_REGISTER_ERROR(BT, bt, CannotSetup, "can't setup", BT_ERR_CANT_INIT);
	DRIVER_REGISTER_ERROR(BT, bt, InvalidMode, "invalid mode", BT_ERR_INVALID_MODE);
	DRIVER_REGISTER_ERROR(BT, bt, NotSetup, "is not setup", BT_ERR_IS_NOT_SETUP);
	DRIVER_REGISTER_ERROR(BT, bt, NotEnoughtMemory, "not enough memory", BT_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(BT, bt, InvalidArgument, "invalid argument", BT_ERR_INVALID_ARGUMENT);
	DRIVER_REGISTER_ERROR(BT, bt, InvalidBeacon, "invalid beacon", BT_ERR_INVALID_BEACON);
	DRIVER_REGISTER_ERROR(BT, bt, CannoStartScan, "can't start scanning", BT_ERR_CANT_START_SCAN);
	DRIVER_REGISTER_ERROR(BT, bt, CannoStopScan, "can't stop scanning", BT_ERR_CANT_STOP_SCAN);
DRIVER_REGISTER_END(BT,bt,NULL,NULL,NULL);

#define evBT_SCAN_START_COMPLETE ( 1 << 0 )
#define evBT_SCAN_START_ERROR    ( 1 << 1 )
#define evBT_SCAN_STOP_COMPLETE  ( 1 << 2 )
#define evBT_SCAN_STOP_ERROR     ( 1 << 3 )

// Is BT setup?
static uint8_t setup = 0;

// Event group to sync some functions with the bt event handler
EventGroupHandle_t bt_event;

// Scan arguments
static esp_ble_scan_params_t scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30
};

/*
 * Helper functions
 */

static void controller_rcv_pkt_ready(void) {
}

static int host_rcv_pkt(uint8_t *data, uint16_t len) {
#if 0
	  printf ("BT (%d):", len);
	  for (int i = 0; i < len; ++i)
	    printf (" %02x", data[i]);
	  printf ("\n");

	if (data[0] == H4_TYPE_EVENT) {
		uint8_t len = data[2];

		printf("event\r\n");
	}
#endif
	return 0;
}

static const esp_vhci_host_callback_t vhci_host_cb = {
    controller_rcv_pkt_ready,
    host_rcv_pkt
};

static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
	switch (event) {
		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
			uint32_t duration = 0;
			esp_ble_gap_start_scanning(duration);
			break;
		}

		case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
			if(param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
				xEventGroupSetBits(bt_event, evBT_SCAN_START_ERROR);
			} else {
				xEventGroupSetBits(bt_event, evBT_SCAN_START_COMPLETE);
			}
			break;
		}

	case ESP_GAP_BLE_SCAN_RESULT_EVT: {
		esp_ble_gap_cb_param_t* scan_result = (esp_ble_gap_cb_param_t*)param;
		switch(scan_result->scan_rst.search_evt)
		{
			case ESP_GAP_SEARCH_INQ_RES_EVT: {
				bt_adv_decode_t data;

				bt_eddystone_decode(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len, &data);
				if (data.frame_type == BTAdvEddystoneUID) {
					printf("BTAdvEddystoneUID\r\n");
				}
				break;
			}
			default:
			break;
		}

		break;
	}
	case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
		if(param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
			xEventGroupSetBits(bt_event, evBT_SCAN_STOP_ERROR);
		} else {
			xEventGroupSetBits(bt_event, evBT_SCAN_STOP_COMPLETE);
		}
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

	bt_event = xEventGroupCreate();

	// Initialize BT controller
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	esp_bt_controller_init(&bt_cfg);

	// Enable BT controller
	if (esp_bt_controller_enable(mode) != ESP_OK) {
		return driver_error(BT_DRIVER, BT_ERR_CANT_INIT, NULL);
	}

	// Register callbacks
	esp_vhci_host_register_callback(&vhci_host_cb);

	// Reset controller
	bt_reset();

	// Set HCI event mask
    uint8_t mask[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x20};
    HCI_Set_Event_Mask(mask);
    HCI_LE_Set_Event_Mask(mask);

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
	driver_error_t *error;

	// Sanity checks
	if (!setup) {
		return driver_error(BT_DRIVER, BT_ERR_IS_NOT_SETUP, NULL);
	}

	if ((error = HCI_LE_Set_Advertise_Enable(0))) return error;
	if ((error = HCI_LE_Set_Advertising_Parameters(adv_params))) return error;
	if ((error = HCI_LE_Set_Advertising_Data(adv_data, adv_data_len))) return error;
	if ((error = HCI_LE_Set_Advertise_Enable(1))) return error;

	return NULL;
}

driver_error_t *bt_adv_stop() {
	driver_error_t *error;

	// Sanity checks
	if (!setup) {
		return driver_error(BT_DRIVER, BT_ERR_IS_NOT_SETUP, NULL);
	}

	if ((error = HCI_LE_Set_Advertise_Enable(0))) return error;

	return NULL;
}

driver_error_t *bt_scan_start() {
	// Sanity checks
	if (!setup) {
		return driver_error(BT_DRIVER, BT_ERR_IS_NOT_SETUP, NULL);
	}

	// Sent scan paraments and start to scan
	esp_ble_gap_set_scan_params(&scan_params);

	EventBits_t uxBits = xEventGroupWaitBits(bt_event, evBT_SCAN_START_COMPLETE | evBT_SCAN_START_ERROR, pdTRUE, pdFALSE, portMAX_DELAY);
	if (uxBits & (evBT_SCAN_START_COMPLETE)) {
		return NULL;
	} else if (uxBits & (evBT_SCAN_START_ERROR)) {
		return driver_error(BT_DRIVER, BT_ERR_CANT_START_SCAN, NULL);
	}

	return NULL;
}

driver_error_t *bt_scan_stop() {
	// Sanity checks
	if (!setup) {
		return driver_error(BT_DRIVER, BT_ERR_IS_NOT_SETUP, NULL);
	}

	esp_ble_gap_stop_scanning();

	EventBits_t uxBits = xEventGroupWaitBits(bt_event, evBT_SCAN_STOP_COMPLETE | evBT_SCAN_STOP_ERROR, pdTRUE, pdFALSE, portMAX_DELAY);
	if (uxBits & (evBT_SCAN_STOP_COMPLETE)) {
		return NULL;
	} else if (uxBits & (evBT_SCAN_STOP_ERROR)) {
		return driver_error(BT_DRIVER, BT_ERR_CANT_STOP_SCAN, NULL);
	}

	return NULL;
}

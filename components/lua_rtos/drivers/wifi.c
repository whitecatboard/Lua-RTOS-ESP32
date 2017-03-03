/*
 * Lua RTOS, WIFI driver
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
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

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_phy_init.h"
#include "tcpip_adapter.h"

#include "rom/rtc.h"

#include <string.h>
#include <stdlib.h>

#include <sys/status.h>
#include <sys/panic.h>
#include <sys/syslog.h>

#include <drivers/wifi.h>

#define WIFI_CONNECT_RETRIES 1
#define WIFI_LOG(m) syslog(LOG_DEBUG, m);

// This macro gets a reference for this driver into drivers array
#define WIFI_DRIVER driver_get_by_name("wifi")

// Driver message errors
DRIVER_REGISTER_ERROR(WIFI, wifi, CannotSetup, "can't setup", WIFI_ERR_CANT_INIT);
DRIVER_REGISTER_ERROR(WIFI, wifi, CannotConnect, "can't connect, review your SSID / password", WIFI_ERR_CANT_CONNECT);
DRIVER_REGISTER_ERROR(WIFI, wifi, GeneralFail, "general fail", WIFI_ERR_WIFI_FAIL);
DRIVER_REGISTER_ERROR(WIFI, wifi, NotEnoughtMemory, "not enough memory", WIFI_ERR_WIFI_NO_MEM);
DRIVER_REGISTER_ERROR(WIFI, wifi, NotSetup, "wifi is not setup", WIFI_ERR_WIFI_NOT_INIT);
DRIVER_REGISTER_ERROR(WIFI, wifi, NotStarted, "wifi is not started", WIFI_ERR_WIFI_NOT_START);
DRIVER_REGISTER_ERROR(WIFI, wifi, InterfaceError, "interface error", WIFI_ERR_WIFI_IF);
DRIVER_REGISTER_ERROR(WIFI, wifi, ModeError, "mode error", WIFI_ERR_WIFI_MODE);
DRIVER_REGISTER_ERROR(WIFI, wifi, InternalError, "internal state error", WIFI_ERR_WIFI_STATE);
DRIVER_REGISTER_ERROR(WIFI, wifi, InternalControlBlockError, "internal control block of station or soft-AP error", WIFI_ERR_WIFI_CONN);
DRIVER_REGISTER_ERROR(WIFI, wifi, InternalNVSError, "internal NVS module error", WIFI_ERR_WIFI_NVS);
DRIVER_REGISTER_ERROR(WIFI, wifi, InvalidMac, "invalid mac address", WIFI_ERR_WIFI_MAC);
DRIVER_REGISTER_ERROR(WIFI, wifi, InvalidSSID, "invalid SSID", WIFI_ERR_WIFI_SSID);
DRIVER_REGISTER_ERROR(WIFI, wifi, InvalidPassword, "invalid password", WIFI_ERR_WIFI_PASSWORD);
DRIVER_REGISTER_ERROR(WIFI, wifi, Timeout, "timeout", WIFI_ERR_WIFI_TIMEOUT);
DRIVER_REGISTER_ERROR(WIFI, wifi, RFClosed, "is in sleep state(RF closed) / wakeup fail", WIFI_ERR_WAKE_FAIL);

// FreeRTOS events used by driver
static EventGroupHandle_t wifiEvent;

#define evWIFI_SCAN_END 	       	 ( 1 << 0 )
#define evWIFI_CONNECTED 	       	 ( 1 << 1 )
#define evWIFI_CANT_CONNECT          ( 1 << 2 )

// Retries for connect
static uint8_t retries = 0;

static driver_error_t *wifi_check_error(esp_err_t error) {
	if (error == ESP_ERR_WIFI_OK) return NULL;

	switch (error) {
		case ESP_ERR_WIFI_FAIL:        return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_FAIL,NULL);
		case ESP_ERR_WIFI_NO_MEM:      return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_NO_MEM,NULL);
		case ESP_ERR_WIFI_NOT_INIT:    return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_NOT_INIT,NULL);
		case ESP_ERR_WIFI_NOT_STARTED: return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_NOT_START,NULL);
		case ESP_ERR_WIFI_IF:          return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_IF,NULL);
		case ESP_ERR_WIFI_STATE:       return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_STATE,NULL);
		case ESP_ERR_WIFI_CONN:        return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_CONN,NULL);
		case ESP_ERR_WIFI_NVS:         return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_NVS,NULL);
		case ESP_ERR_WIFI_MAC:         return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_MAC,NULL);
		case ESP_ERR_WIFI_SSID:        return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_SSID,NULL);
		case ESP_ERR_WIFI_PASSWORD:    return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_PASSWORD,NULL);
		case ESP_ERR_WIFI_TIMEOUT:     return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_TIMEOUT,NULL);
		case ESP_ERR_WIFI_WAKE_FAIL:   return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WAKE_FAIL,NULL);
		default:
			panic("missing wifi error case");
	}

	return NULL;
}

static esp_err_t event_handler(void *ctx, system_event_t *event) {
	switch (event->event_id) {
		case SYSTEM_EVENT_STA_START:
			WIFI_LOG("SYSTEM_EVENT_STA_START\n");
			esp_wifi_connect();
			break;

		case SYSTEM_EVENT_STA_STOP:
			WIFI_LOG("SYSTEM_EVENT_STA_STOP\n");
			break;

	    case SYSTEM_EVENT_STA_DISCONNECTED:
	    	WIFI_LOG("SYSTEM_EVENT_STA_DISCONNECTED\n");

			if (!status_get(STATUS_WIFI_CONNECTED)) {
				if (retries > WIFI_CONNECT_RETRIES) {
					status_clear(STATUS_WIFI_CONNECTED);
					xEventGroupSetBits(wifiEvent, evWIFI_CANT_CONNECT);
					break;
				} else {
					retries++;

					status_clear(STATUS_WIFI_CONNECTED);
					esp_wifi_connect();
				}
			}

			status_clear(STATUS_WIFI_CONNECTED);

			esp_wifi_connect();
	    	break;

		case SYSTEM_EVENT_STA_GOT_IP:
			WIFI_LOG("SYSTEM_EVENT_STA_GOT_IP\n");
 		    xEventGroupSetBits(wifiEvent, evWIFI_CONNECTED);
			break;

		case SYSTEM_EVENT_AP_STA_GOT_IP6:
			WIFI_LOG("SYSTEM_EVENT_AP_STA_GOT_IP6\n");
 		    xEventGroupSetBits(wifiEvent, evWIFI_CONNECTED);
			break;

		case SYSTEM_EVENT_SCAN_DONE:
			WIFI_LOG("SYSTEM_EVENT_SCAN_DONE\n");
 		    xEventGroupSetBits(wifiEvent, evWIFI_SCAN_END);
			break;

		case SYSTEM_EVENT_STA_CONNECTED:
			WIFI_LOG("SYSTEM_EVENT_STA_CONNECTED\n");
			status_set(STATUS_WIFI_CONNECTED);
			break;

		default :
			printf("other event %d\r\n", event->event_id);
			break;
	}

   return ESP_OK;
}

static driver_error_t *wifi_init(wifi_mode_t mode) {
	driver_error_t *error;

	if (!status_get(STATUS_WIFI_INITED)) {
		wifiEvent = xEventGroupCreate();

		if (!status_get(STATUS_TCPIP_INITED)) {
			tcpip_adapter_init();

			status_set(STATUS_TCPIP_INITED);
		}

		esp_event_loop_init(event_handler, NULL);

		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		if ((error = wifi_check_error(esp_wifi_init(&cfg)))) return error;
		if ((error = wifi_check_error(esp_wifi_set_storage(WIFI_STORAGE_RAM)))) return error;
		if ((error = wifi_check_error(esp_wifi_set_mode(mode)))) return error;

		status_set(STATUS_WIFI_INITED);
	}

	return NULL;
}

static driver_error_t *wifi_deinit() {
	// TO DO
	// esp_wifi_dinit: This API can not be called yet and will be done in the future.
	#if 0
	driver_error_t *error;
	if (status_get(STATUS_WIFI_INITED)) {
		// Remove and stop wifi driver from system
		if ((error = wifi_check_error(esp_wifi_deinit()))) return error;

		// Remove event group
		vEventGroupDelete(wifiEvent);

		status_clear(STATUS_WIFI_INITED);
	}
	#endif

	return NULL;
}

driver_error_t *wifi_scan(uint16_t *count, wifi_ap_record_t **list) {
	driver_error_t *error;

	if (!status_get(STATUS_WIFI_SETUP)) {
		// Attach wifi driver
		if ((error = wifi_init(WIFI_MODE_STA))) {
			return error;
		}
	}

	if (!status_get(STATUS_WIFI_STARTED)) {
		// Start wifi
		if ((error = wifi_check_error(esp_wifi_start()))) return error;
	}

	wifi_scan_config_t conf = {
		.ssid = NULL,
		.bssid = NULL,
		.channel = 0,
		.show_hidden = 1
	};

	// Start scan
	if ((error = wifi_check_error(esp_wifi_scan_start(&conf, true)))) return error;

	// Wait for scan end
    EventBits_t uxBits = xEventGroupWaitBits(wifiEvent, evWIFI_SCAN_END, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & (evWIFI_SCAN_END)) {
    	// Get count of finded AP
    	if ((error = wifi_check_error(esp_wifi_scan_get_ap_num(count)))) return error;

    	// Allocate space for AP list
    	*list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * (*count));
    	if (!*list) {
    		return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_NO_MEM,NULL);
    	}

    	// Get AP list
    	if ((error = wifi_check_error(esp_wifi_scan_get_ap_records(count, *list)))) {
    		free(list);

    		return error;
    	}
    }

	if (!status_get(STATUS_WIFI_STARTED)) {
		// Stop wifi
		if ((error = wifi_check_error(esp_wifi_stop()))) return error;
	}

	if (!status_get(STATUS_WIFI_SETUP)) {
		// Detach wifi driver
		if ((error = wifi_deinit())) return error;
	}

	return NULL;
}

driver_error_t *wifi_setup(wifi_mode_t mode, char *ssid, char *password) {
	driver_error_t *error;

	status_clear(STATUS_WIFI_SETUP);

	// Attach wifi driver
	if ((error = wifi_init(mode))) return error;

	// Setup mode and config related to desired mode
	if (mode == WIFI_MODE_STA) {
	    wifi_config_t wifi_config;

	    memset(&wifi_config, 0, sizeof(wifi_config_t));

	    strncpy((char *)wifi_config.sta.ssid, ssid, 32);
	    strncpy((char *)wifi_config.sta.password, password, 64);

	    // Set config
	    if ((error = wifi_check_error(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config)))) return error;
	} else {
		return driver_setup_error(WIFI_DRIVER, WIFI_ERR_CANT_INIT, "invalid mode");
	}

	status_set(STATUS_WIFI_SETUP);

	return NULL;
}

driver_error_t *wifi_start() {
	driver_error_t *error;

	if (!status_get(STATUS_WIFI_SETUP)) {
		return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_NOT_INIT, NULL);
	}

	if (!status_get(STATUS_WIFI_STARTED)) {
		retries = 0;

		if ((error = wifi_check_error(esp_wifi_start()))) return error;

	    EventBits_t uxBits = xEventGroupWaitBits(wifiEvent, evWIFI_CONNECTED | evWIFI_CANT_CONNECT, pdTRUE, pdFALSE, portMAX_DELAY);
	    if (uxBits & (evWIFI_CONNECTED)) {
		    status_set(STATUS_WIFI_STARTED);
	    }

	    if (uxBits & (evWIFI_CANT_CONNECT)) {
	    	esp_wifi_stop();

			return driver_operation_error(WIFI_DRIVER, WIFI_ERR_CANT_CONNECT, NULL);
	    }
	}

	return NULL;
}

driver_error_t *wifi_stop() {
	driver_error_t *error;

	if (!status_get(STATUS_WIFI_SETUP)) {
		return driver_operation_error(WIFI_DRIVER, WIFI_ERR_WIFI_NOT_INIT, NULL);
	}

	if (status_get(STATUS_WIFI_STARTED)) {
		if ((error = wifi_check_error(esp_wifi_stop()))) return error;

		status_clear(STATUS_WIFI_STARTED);
	}

	return NULL;
}

driver_error_t *wifi_stat(ifconfig_t *info) {
	tcpip_adapter_ip_info_t esp_info;
	uint8_t mac[6];

	driver_error_t *error;

	// Get WIFI IF info
	if ((error = wifi_check_error(tcpip_adapter_get_ip_info(ESP_IF_WIFI_STA, &esp_info)))) return error;

	// Get MAC info
	if ((error = wifi_check_error(esp_wifi_get_mac(ESP_IF_WIFI_STA, mac)))) return error;

	// Copy info
	info->gw = esp_info.gw;
	info->ip = esp_info.ip;
	info->netmask = esp_info.netmask;

	memcpy(info->mac, mac, sizeof(mac));

	return NULL;
}

DRIVER_REGISTER(WIFI,wifi,NULL,NULL,NULL);

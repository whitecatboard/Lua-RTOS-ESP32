/*
 * Lua RTOS, network manager
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

#if CONFIG_LUA_RTOS_LUA_USE_NET

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_ota_ops.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "openssl/ssl.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/status.h>
#include <sys/syslog.h>

#include <drivers/net.h>
#include <drivers/net_http.h>
#include <drivers/wifi.h>

#include <lwip/ping.h>

// Register drivers and errors
DRIVER_REGISTER_BEGIN(NET,net,NULL,NULL,NULL);
	DRIVER_REGISTER_ERROR(NET, net, NotAvailable, "network is not available", NET_ERR_NOT_AVAILABLE);
	DRIVER_REGISTER_ERROR(NET, net, InvalidIpAddr, "invalid IP adddress", NET_ERR_INVALID_IP);
	DRIVER_REGISTER_ERROR(NET, net, NoMoreCallbacksAvailable, "no more callbacks available", NET_ERR_NO_MORE_CALLBACKS);
	DRIVER_REGISTER_ERROR(NET, net, CallbackNotFound, "callback not found", NET_ERR_NO_CALLBACK_NOT_FOUND);
	DRIVER_REGISTER_ERROR(NET, net, NameCannotBeResolved, "name cannot be resolved", NET_ERR_NAME_CANNOT_BE_RESOLVED);
	DRIVER_REGISTER_ERROR(NET, net, CannotCreateSocket, "cannot create socket", NET_ERR_NAME_CANNOT_CREATE_SOCKET);
	DRIVER_REGISTER_ERROR(NET, net, CannotSetupSocket, "cannot setup socket", NET_ERR_NAME_CANNOT_SETUP_SOCKET);
	DRIVER_REGISTER_ERROR(NET, net, CannotConnect, "cannot connect to server", NET_ERR_NAME_CANNOT_CONNECT);
	DRIVER_REGISTER_ERROR(NET, net, CannotCreateSSL, "cannot create SSL", NET_ERR_CANNOT_CREATE_SSL);
	DRIVER_REGISTER_ERROR(NET, net, CannotConnectSSL, "cannot connect SSL", NET_ERR_CANNOT_CONNECT_SSL);
	DRIVER_REGISTER_ERROR(NET, net, NotEnoughtMemory, "not enough memory", NET_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(NET, net, InvalidResponse, "invalid response", NET_ERR_INVALID_RESPONSE);
	DRIVER_REGISTER_ERROR(NET, net, InvalidContent, "invalid content", NET_ERR_INVALID_CONTENT);
DRIVER_REGISTER_END(NET,net,NULL,NULL,NULL);

// FreeRTOS events used by driver
EventGroupHandle_t netEvent;

// Retries for connect
static uint8_t retries = 0;

// Event callbacks
static net_event_register_callback_t callback[MAX_NET_EVENT_CALLBACKS] = {0};

/*
 * Helper functions
 */
static esp_err_t event_handler(void *ctx, system_event_t *event) {
	switch (event->event_id) {
		case SYSTEM_EVENT_WIFI_READY: 	            /**< ESP32 WiFi ready */
			break;

		case SYSTEM_EVENT_SCAN_DONE:                /**< ESP32 finish scanning AP */
 			xEventGroupSetBits(netEvent, evWIFI_SCAN_END);
			break;

		case SYSTEM_EVENT_STA_START:                /**< ESP32 station start */
			esp_wifi_connect();
			break;

		case SYSTEM_EVENT_STA_STOP:                 /**< ESP32 station stop */
			break;

		case SYSTEM_EVENT_STA_CONNECTED:            /**< ESP32 station connected to AP */
			status_set(STATUS_WIFI_CONNECTED);
			tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
			break;

		case SYSTEM_EVENT_STA_DISCONNECTED:         /**< ESP32 station disconnected from AP */
			if (!status_get(STATUS_WIFI_CONNECTED)) {
				if (retries > WIFI_CONNECT_RETRIES) {
					status_clear(STATUS_WIFI_CONNECTED);
					xEventGroupSetBits(netEvent, evWIFI_CANT_CONNECT);
					retries = 0;
					break;
				} else {
					retries++;

					status_clear(STATUS_WIFI_CONNECTED);
					esp_wifi_connect();
				}
			}

			status_clear(STATUS_WIFI_CONNECTED);
			break;

		case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:      /**< the auth mode of AP connected by ESP32 station changed */
			break;

		case SYSTEM_EVENT_STA_GOT_IP:               /**< ESP32 station got IP from connected AP */
 			xEventGroupSetBits(netEvent, evWIFI_CONNECTED);
			break;

		case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:       /**< ESP32 station wps succeeds in enrollee mode */
			break;

		case SYSTEM_EVENT_STA_WPS_ER_FAILED:        /**< ESP32 station wps fails in enrollee mode */
			break;

		case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:       /**< ESP32 station wps timeout in enrollee mode */
			break;

		case SYSTEM_EVENT_STA_WPS_ER_PIN:           /**< ESP32 station wps pin code in enrollee mode */
			break;

		case SYSTEM_EVENT_AP_START:                 /**< ESP32 soft-AP start */
			status_set(STATUS_WIFI_CONNECTED);
			tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_AP);
			break;

		case SYSTEM_EVENT_AP_STOP:                  /**< ESP32 soft-AP stop */
			status_clear(STATUS_WIFI_CONNECTED);
			status_clear(STATUS_WIFI_INITED);
			break;

		case SYSTEM_EVENT_AP_STACONNECTED:          /**< a station connected to ESP32 soft-AP */
			break;

		case SYSTEM_EVENT_AP_STADISCONNECTED:       /**< a station disconnected from ESP32 soft-AP */
			break;

		case SYSTEM_EVENT_AP_PROBEREQRECVED:        /**< Receive probe request packet in soft-AP interface */
			break;

		case SYSTEM_EVENT_AP_STA_GOT_IP6:           /**< ESP32 station or ap interface v6IP addr is preferred */
 			xEventGroupSetBits(netEvent, evWIFI_CONNECTED);
			break;

		case SYSTEM_EVENT_ETH_START:                /**< ESP32 ethernet start */
			break;

		case SYSTEM_EVENT_ETH_STOP:                 /**< ESP32 ethernet stop */
			break;

		case SYSTEM_EVENT_ETH_CONNECTED:            /**< ESP32 ethernet phy link up */
			tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_ETH);
			break;

		case SYSTEM_EVENT_ETH_DISCONNECTED:         /**< ESP32 ethernet phy link down */
			break;

		case SYSTEM_EVENT_ETH_GOT_IP:               /**< ESP32 ethernet got IP from connected AP */
			break;
#if CONFIG_SPI_ETHERNET
		case SYSTEM_EVENT_SPI_ETH_START:            /**< ESP32 spi ethernet start */
			break;

		case SYSTEM_EVENT_SPI_ETH_STOP:             /**< ESP32 spi ethernet stop */
			break;

		case SYSTEM_EVENT_SPI_ETH_CONNECTED:        /**< ESP32 spi ethernet phy link up */
			status_set(STATUS_SPI_ETH_CONNECTED);
			tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_SPI_ETH);
			break;

		case SYSTEM_EVENT_SPI_ETH_DISCONNECTED:     /**< ESP32 spi ethernet phy link down */
			status_clear(STATUS_SPI_ETH_CONNECTED);
			break;

		case SYSTEM_EVENT_SPI_ETH_GOT_IP:           /**< ESP32 spi ethernet got IP from connected AP */
 			xEventGroupSetBits(netEvent, evSPI_ETH_CONNECTED);
			break;
#endif
		default :
			break;
	}

	// Call to the registered callbacks
	int i = 0;

	for(i=0; i < MAX_NET_EVENT_CALLBACKS; i++) {
		if (callback[i]) {
			callback[i](event);
		}
	}

   return ESP_OK;
}

/*
 * Operation functions
 */
driver_error_t *net_init() {
	if (!status_get(STATUS_TCPIP_INITED)) {
		status_set(STATUS_TCPIP_INITED);

		retries = 0;

		netEvent = xEventGroupCreate();

		tcpip_adapter_init();

		esp_event_loop_init(event_handler, NULL);
	}

	return NULL;
}

driver_error_t *net_check_connectivity() {
	if (!NETWORK_AVAILABLE()) {
		return driver_error(NET_DRIVER, NET_ERR_NOT_AVAILABLE,NULL);
	}

	return NULL;
}

driver_error_t *net_lookup(const char *name, int port, struct sockaddr_in *address) {
	driver_error_t *error;
	int rc = 0;

	if ((error = net_check_connectivity())) return error;

	sa_family_t family = AF_INET;
	struct addrinfo *result = NULL;
	struct addrinfo hints = {0, AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

	if ((rc = getaddrinfo(name, NULL, &hints, &result)) == 0) {
		struct addrinfo *res = result;
		while (res) {
			if (res->ai_family == AF_INET) {
				result = res;
				break;
			}
			res = res->ai_next;
		}

		if (result->ai_family == AF_INET) {
			address->sin_port = htons(port);
			address->sin_family = family = AF_INET;
			address->sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
		}

		freeaddrinfo(result);
	} else {
		return driver_error(NET_DRIVER, NET_ERR_NAME_CANNOT_BE_RESOLVED,NULL);
	}

	return NULL;
}

driver_error_t *net_event_register_callback(net_event_register_callback_t func) {
	int i = 0;

	for(i=0; i < MAX_NET_EVENT_CALLBACKS; i++) {
		if (callback[i] == 0) {
			callback[i] = func;
			return NULL;
		}
	}

	return driver_error(NET_DRIVER, NET_ERR_NO_MORE_CALLBACKS,NULL);
}

driver_error_t *net_event_unregister_callback(net_event_register_callback_t func) {
	int i = 0;

	for(i=0; i < MAX_NET_EVENT_CALLBACKS; i++) {
		if (callback[i] == func) {
			callback[i] = NULL;
			return NULL;
		}
	}

	return driver_error(NET_DRIVER, NET_ERR_NO_MORE_CALLBACKS,NULL);
}

driver_error_t *net_ping(const char *name, int count, int interval, int size, int timeout) {
	driver_error_t *error;

	if ((error = net_check_connectivity())) return error;

	if ((error = ping(name, count, interval, size, timeout))) return error;

	return NULL;
}

driver_error_t *net_reconnect() {
	return NULL;
}

driver_error_t *net_ota() {
	driver_error_t *error;
	net_http_client_t client = HTTP_CLIENT_INITIALIZER;
	net_http_response_t response;
	esp_ota_handle_t update_handle = 0 ;
	uint8_t buffer[1024];

    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    if (!update_partition) {
	    syslog(LOG_ERR,
			"OTA partition not found. You need an OTA compatible firmware."
		);

	    return NULL;
    }

    if ((error = net_check_connectivity())) {
    	return error;
    }

    syslog(LOG_INFO, "Connecting to https://ota.whitecatboard.org ...");
	if ((error = net_http_create_client("ota.whitecatboard.org", "443", &client))) {
		return error;
	}

    syslog(LOG_INFO, "Current firmware commit is %s", BUILD_COMMIT);
    sprintf((char *)buffer, "/?board=%s&commit=%s", LUA_RTOS_BOARD, BUILD_COMMIT);
	if ((error = net_http_get(&client, (const char *)buffer, &response))) {
		return error;
	}

	if ((response.code == 200) && (response.size > 0)) {
	    syslog(LOG_INFO, "New firmware available");

	    syslog(LOG_INFO,
			"Running partition is %s, at offset 0x%08x",
			 running->label, running->address
		);

	    syslog(LOG_INFO,
	    	"Writing partition is %s, at offset 0x%x",
			update_partition->label, update_partition->address
		);

	    syslog(LOG_INFO, "Begin OTA update ...");

	    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
	    if (err != ESP_OK) {
	        syslog(LOG_ERR, "Failed, error %d", err);
	    }

	    uint32_t address = update_partition->address;

		while (response.size > 0){
			if ((error = net_http_read_response(&response, buffer, sizeof(buffer)))) {
				return error;
			}

            err = esp_ota_write(update_handle, buffer, response.len);
            if (err != ESP_OK) {
    		    syslog(LOG_ERR, "\nChunk written unsuccessfully in partition (offset 0x%08x), error %d", address, err);
                return NULL;
            } else {
            	printf("\rChunk written successfully in partition at offset 0x%08x", address);
            }

            address = address + response.len;
		}

		syslog(LOG_INFO, "\nEnding OTA update ...");

	    if (esp_ota_end(update_handle) != ESP_OK) {
		    syslog(LOG_ERR, "Failed");
	    } else {
		    syslog(LOG_INFO, "Changing boot partition ...");
		    err = esp_ota_set_boot_partition(update_partition);
		    if (err != ESP_OK) {
			    syslog(LOG_ERR, "Failed, err %d", err);
		    } else {
			    syslog(LOG_INFO, "Updated");
		    }
	    }
	} else if (response.code == 470) {
	    syslog(LOG_INFO, "Missing or bad arguments");
	} else if (response.code == 471) {
	    syslog(LOG_INFO, "No new firmware available");
	}

	if ((error = net_http_destroy_client(&client))) {
		return error;
	}

	if (response.code == 200) {
		syslog(LOG_INFO, "Restarting ...");
		esp_restart();
	}

	return NULL;
}

#endif

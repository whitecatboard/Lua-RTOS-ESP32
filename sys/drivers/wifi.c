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
 * Lua RTOS, wifi driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_phy_init.h"

#include "rom/rtc.h"

#include "lwip/dns.h"
#include "lwip/ip_addr.h"

#include <string.h>
#include <stdlib.h>

#include <sys/delay.h>
#include <sys/status.h>
#include <sys/panic.h>
#include <sys/syslog.h>

#include <drivers/wifi.h>
//#include <esp_wps.h>
//#include <esp_wpa2.h>
//#include "esp_smartconfig.h"

#include <pthread.h>

#define WIFI_LOG(...) syslog(LOG_DEBUG, __VA_ARGS__);

#define WIFI_DRIVER_SCAN_START_TIMEOUT (500 / portTICK_PERIOD_MS)

// This macro gets a reference for this driver into drivers array
#define WIFI_DRIVER driver_get_by_name("wifi")

DRIVER_REGISTER_BEGIN(WIFI,wifi,0,NULL,NULL);
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
    DRIVER_REGISTER_ERROR(WIFI, wifi, InvalidArg, "invalid argument", WIFI_ERR_INVALID_ARGUMENT);
    DRIVER_REGISTER_ERROR(WIFI, wifi, NotSupport, "wifi API is not supported yet", WIFI_ERR_NOT_SUPPORT);
    DRIVER_REGISTER_ERROR(WIFI, wifi, NotStopped, "driver was not stopped", WIFI_ERR_NOT_STOPPED);
DRIVER_REGISTER_END(WIFI,wifi,0,NULL,NULL);

extern EventGroupHandle_t netEvent;
extern net_event_register_callback_t net_event_callback[MAX_NET_EVENT_CALLBACKS];

// TO DO
#if 0
static int wps_mode = WPS_TYPE_DISABLE;
static esp_wps_config_t * wps_config = NULL;
static wifi_wps_pin_cb * wps_pin_callback = NULL;
static wifi_sc_cb * wps_sc_callback = NULL;

#define WPSPIN2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5], (a)[6], (a)[7]
#endif

static esp_netif_t *netif = NULL;

// Retries for connect
static uint8_t connect_retries = 0;

static void net_wifi_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    EventBits_t bits = 0;

    // Exit if not for us
    if (((esp_event_base_t)arg) != WIFI_EVENT) {
		return;
	}

    switch (id) {
        case WIFI_EVENT_WIFI_READY: // SP32 WiFi ready
            break;

        case WIFI_EVENT_SCAN_DONE: // ESP32 finish scanning AP
            bits |= evWIFI_SCAN_END;
            break;

        // STA events
        case WIFI_EVENT_STA_START: // ESP32 station start
            status_set(0x00000000, STATUS_WIFI_CONNECTED | STATUS_WIFI_HAS_IP);
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_STOP: // ESP32 station stop
            status_set(0x00000000, STATUS_WIFI_CONNECTED | STATUS_WIFI_HAS_IP);
            break;

        case WIFI_EVENT_STA_CONNECTED: // ESP32 station connected to AP
            status_set(STATUS_WIFI_CONNECTED, 0x00000000);
            bits |= evWIFI_STARTED;
            // TO DO
            //tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
            break;

        case WIFI_EVENT_STA_DISCONNECTED: // ESP32 station disconnected from AP */
            if (status_get(STATUS_WIFI_SYNC) && (connect_retries > WIFI_CONNECT_RETRIES)) {
                bits |= evWIFI_CANT_CONNECT;
                status_set(0x00000000, STATUS_WIFI_CONNECTED);
                connect_retries = 0;
            } else {
                status_set(0x00000000, STATUS_WIFI_CONNECTED);
                if (status_get(STATUS_WIFI_STARTED)) {
                    if (status_get(STATUS_WIFI_SYNC)) {
                        connect_retries++;
                    }
                    delay(200);
                    esp_wifi_connect();
                }
            }
            break;

        case WIFI_EVENT_STA_AUTHMODE_CHANGE: // The auth mode of AP connected by ESP32 station changed */
            break;

        // STA WPS events
        case WIFI_EVENT_STA_WPS_ER_SUCCESS: // ESP32 station wps succeeds in enrollee mode */
            wifi_wps_disable();
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_WPS_ER_FAILED: // ESP32 station wps fails in enrollee mode
            wifi_wps_reconnect();
            break;

        case WIFI_EVENT_STA_WPS_ER_TIMEOUT:       /**< ESP32 station wps timeout in enrollee mode */
            wifi_wps_reconnect();
            break;

        case WIFI_EVENT_STA_WPS_ER_PIN:           /**< ESP32 station wps pin code in enrollee mode */
            // TO DO
        	// wifi_wps_pin(event->event_info.sta_er_pin.pin_code);
            break;

        case WIFI_EVENT_AP_START:                 /**< ESP32 soft-AP start */
        	// TO DO
            // tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_AP);
            break;

        case WIFI_EVENT_AP_STOP:                  /**< ESP32 soft-AP stop */
            status_set(0x00000000, STATUS_WIFI_CONNECTED | STATUS_WIFI_INITED);
            break;

        case WIFI_EVENT_AP_STACONNECTED:          /**< a station connected to ESP32 soft-AP */
            break;

        case WIFI_EVENT_AP_STADISCONNECTED:       /**< a station disconnected from ESP32 soft-AP */
            break;

        case WIFI_EVENT_AP_PROBEREQRECVED:        /**< Receive probe request packet in soft-AP interface */
            break;
    }

    // Call to the registered callbacks
    for(int i=0; i < MAX_NET_EVENT_CALLBACKS; i++) {
        if (net_event_callback[i]) {
			net_event_callback[i](NetEventTypeWifi, id);
        }
    }

    if (bits) {
        xEventGroupSetBits(netEvent, bits);
    }
}

static void net_wifi_ip_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    EventBits_t bits = 0;

    // Exit if not for us
    if (((esp_event_base_t)arg) != IP_EVENT) {
		return;
	}

	switch (id) {
		case IP_EVENT_STA_GOT_IP:
            status_set(STATUS_WIFI_HAS_IP, 0x00000000);
            bits |= evWIFI_CONNECTED;
            break;

		case IP_EVENT_STA_LOST_IP:
            status_set(0x00000000, STATUS_WIFI_HAS_IP);
            break;

		case IP_EVENT_AP_STAIPASSIGNED:
			break;

		case IP_EVENT_GOT_IP6:
            bits |= evWIFI_CONNECTED;
			break;
	}

    // Call to the registered callbacks
    for(int i=0; i < MAX_NET_EVENT_CALLBACKS; i++) {
        if (net_event_callback[i]) {
			net_event_callback[i](NetEventTypeWifiIp, id);
        }
    }

    if (bits) {
        xEventGroupSetBits(netEvent, bits);
    }
}

driver_error_t *wifi_check_error(esp_err_t error) {
    if (error == ESP_OK) return NULL;

    switch (error) {
        case ESP_FAIL:                 return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_FAIL,NULL);
        case ESP_ERR_NO_MEM:           return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_NO_MEM,NULL);
        case ESP_ERR_WIFI_NOT_INIT:    return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_NOT_INIT,NULL);
        case ESP_ERR_WIFI_NOT_STARTED: return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_NOT_START,NULL);
        case ESP_ERR_WIFI_IF:          return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_IF,NULL);
        case ESP_ERR_WIFI_STATE:       return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_STATE,NULL);
        case ESP_ERR_WIFI_CONN:        return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_CONN,NULL);
        case ESP_ERR_WIFI_NVS:         return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_NVS,NULL);
        case ESP_ERR_WIFI_MAC:         return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_MAC,NULL);
        case ESP_ERR_WIFI_SSID:        return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_SSID,NULL);
        case ESP_ERR_WIFI_PASSWORD:    return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_PASSWORD,NULL);
        case ESP_ERR_WIFI_TIMEOUT:     return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_TIMEOUT,NULL);
        case ESP_ERR_WIFI_WAKE_FAIL:   return driver_error(WIFI_DRIVER, WIFI_ERR_WAKE_FAIL,NULL);
        case ESP_ERR_WIFI_MODE:        return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_MODE,NULL);
        case ESP_ERR_INVALID_ARG:      return driver_error(WIFI_DRIVER, WIFI_ERR_INVALID_ARGUMENT,NULL);
        case ESP_ERR_NOT_SUPPORTED:    return driver_error(WIFI_DRIVER, WIFI_ERR_NOT_SUPPORT,NULL);
        case ESP_ERR_WIFI_NOT_STOPPED: return driver_error(WIFI_DRIVER, WIFI_ERR_NOT_STOPPED,NULL);

        default: {
            char *buffer;

            buffer = malloc(40);
            if (!buffer) {
                panic("not enough memory");
            }

            snprintf(buffer, 40, "missing wifi error case %d", error);

            return driver_error(WIFI_DRIVER, WIFI_ERR_CANT_INIT, buffer);
        }
    }

    return NULL;
}

static driver_error_t *wifi_init(wifi_mode_t mode) {
    driver_error_t *error;

    connect_retries = 0;

    if ((error = net_init())) {
        return error;
    }

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &net_wifi_handler, WIFI_EVENT, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &net_wifi_ip_handler, IP_EVENT, NULL);

    if (!status_get(STATUS_WIFI_INITED)) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        if ((error = wifi_check_error(esp_wifi_init(&cfg)))) return error;

#if CONFIG_ESP32_WIFI_NVS_ENABLED
        wifi_storage_t storage = WIFI_STORAGE_FLASH;
#else
        wifi_storage_t storage = WIFI_STORAGE_RAM;
#endif

        if ((error = wifi_check_error(esp_wifi_set_storage(storage)))) return error;

        if ((error = wifi_check_error(esp_wifi_set_mode(mode)))) return error;
//        wifi_country_t country = { "EU", 1, 13, WIFI_COUNTRY_POLICY_AUTO };
  //      if ((error = wifi_check_error(esp_wifi_set_country(&country)))) return error;

        status_set(STATUS_WIFI_INITED, 0x00000000);
    }

    return NULL;
}

static driver_error_t *wifi_deinit() {
    driver_error_t *error;
    if (status_get(STATUS_WIFI_INITED)) {
        // Remove and stop wifi driver from system
        if ((error = wifi_check_error(esp_wifi_deinit()))) return error;

        // doesn't seem to be recreated so DON'T remove the event group
        // vEventGroupDelete(netEvent);

        status_set(0x00000000, STATUS_WIFI_INITED);
    }

    return NULL;
}

driver_error_t *wifi_scan(uint16_t *count, wifi_ap_record_t **list) {
    driver_error_t *error;

    *list = NULL;
    *count = 0;

    if (status_get(STATUS_WIFI_INITED)) {
        wifi_mode_t mode;
        if ((error = wifi_check_error(esp_wifi_get_mode(&mode)))) return error;

        if (WIFI_MODE_AP == mode) {
            if(status_get(STATUS_WIFI_STARTED)) {
                if ((error = wifi_check_error(esp_wifi_stop()))) return error;
                status_set(0x00000000, STATUS_WIFI_STARTED);
            }
            status_set(0x00000000, STATUS_WIFI_INITED);
        }
    }

    if (!status_get(STATUS_WIFI_INITED)) {
        // Attach wifi driver
        if ((error = wifi_init(WIFI_MODE_STA))) {
            return error;
        }
    }

    if (!status_get(STATUS_WIFI_STARTED)) {
        // Start wifi
        if ((error = wifi_check_error(esp_wifi_start()))) return error;

        EventBits_t uxBits = xEventGroupWaitBits(netEvent, evWIFI_STARTED, pdTRUE, pdFALSE, WIFI_DRIVER_SCAN_START_TIMEOUT);
        if (uxBits & (evWIFI_STARTED)) {
        } else {
        	return driver_error(WIFI_DRIVER, NET_ERR_TIMEOUT,NULL);
        }
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
    EventBits_t uxBits = xEventGroupWaitBits(netEvent, evWIFI_SCAN_END, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & (evWIFI_SCAN_END)) {
        // Get count of found AP
        if ((error = wifi_check_error(esp_wifi_scan_get_ap_num(count)))) return error;

        // An empty list shall not throw an error
        if(0 < *count) {

            // Allocate space for AP list
            *list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * (*count));
            if (!*list) {
                return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_NO_MEM,NULL);
            }

            // Get AP list
            if ((error = wifi_check_error(esp_wifi_scan_get_ap_records(count, *list)))) {
                *list = NULL;
                *count = 0;
                free(list);

                return error;
            }

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

driver_error_t *wifi_setup(wifi_mode_t mode, char *ssid, char *password, uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns1, uint32_t dns2, int powersave, int channel, int hidden) {
    driver_error_t *error;
    wifi_interface_t interface;

    status_set(0x00000000, STATUS_WIFI_SETUP);

    // Sanity checks
    if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
        if (*password && strlen(password) < 8) {
            return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_PASSWORD, "if provided the password must have more than 7 characters");
        }
    }

    if (status_get(STATUS_WIFI_INITED)) {
        wifi_mode_t curmode;
        if ((error = wifi_check_error(esp_wifi_get_mode(&curmode)))) return error;
        if (curmode != mode) {
            //in case of switching mode AP<->STA Stop wifi
            if(status_get(STATUS_WIFI_STARTED)) {
                if ((error = wifi_check_error(esp_wifi_stop()))) return error;
                status_set(0x00000000, STATUS_WIFI_STARTED);
            }
            status_set(0x00000000, STATUS_WIFI_INITED);
        }
    }

    // Attach wifi driver
    if ((error = wifi_init(mode))) return error;

    esp_netif_init();

    if (mode == WIFI_MODE_STA) {
    	netif = esp_netif_create_default_wifi_sta();

        // Setup mode and config related to desired mode
        wifi_config_t wifi_config;
        memset(&wifi_config, 0, sizeof(wifi_config_t));

        strncpy((char *)wifi_config.sta.ssid, ssid, 32);
        strncpy((char *)wifi_config.sta.password, password, 64);

        wifi_config.sta.channel = (channel ? channel : 0);

        interface = ESP_IF_WIFI_STA;
        if ((error = wifi_check_error(esp_wifi_set_config(interface, &wifi_config)))) return error;

        // TO DO
    	#if 0
        if ((error = wifi_check_error(esp_wifi_sta_wpa2_ent_disable()))) return error;
        WIFI_LOG("wpa2 enterprise disabled\n");
		#endif
    }
    if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
    	netif = esp_netif_create_default_wifi_sta();

        // Setup mode and config related to desired mode
        wifi_config_t wifi_config;
        memset(&wifi_config, 0, sizeof(wifi_config_t));

        strncpy((char *)wifi_config.ap.ssid, ssid, 32);
        strncpy((char *)wifi_config.ap.password, password, 64);

        wifi_config.ap.ssid_len = 0;
        wifi_config.ap.channel = (channel ? channel : 1);

        // TO DO
		#if 0
        wifi_config.ap.authmode = (*password ? WIFI_AUTH_WPA_WPA2_PSK : WIFI_AUTH_OPEN);
		#else
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
		#endif

        wifi_config.ap.ssid_hidden = hidden;
        wifi_config.ap.max_connection = 4;
        wifi_config.ap.beacon_interval = 100;

        interface = ESP_IF_WIFI_AP;
        if ((error = wifi_check_error(esp_wifi_set_config(interface, &wifi_config)))) return error;
    }

    if (powersave)
        if ((error = wifi_check_error(esp_wifi_set_ps(powersave)))) return error;

    status_set(STATUS_WIFI_SETUP, 0x00000000);

    // Set ip / mask / gw, if present
    if (ip && mask && gw) {
    	esp_netif_ip_info_t ip_info;
    	esp_netif_dns_info_t dns_info;

        ip_info.ip.addr = ip;
        ip_info.netmask.addr = mask;
        ip_info.gw.addr = gw;

        esp_netif_dhcpc_stop(netif);
        esp_netif_set_ip_info(netif, &ip_info);

        // If present, set dns1, else set to 8.8.8.8
        if (!dns1) dns1 = 134744072;

        dns_info.ip.u_addr.ip4.addr = dns1;
        dns_info.ip.type = IPADDR_TYPE_V4;

        esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);

        // If present, set dns2, else set to 8.8.4.4
        if (!dns2) dns2 = 67373064;

        dns_info.ip.u_addr.ip4.addr = dns2;
        dns_info.ip.type = IPADDR_TYPE_V4;

        esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
    }
    return NULL;
}

driver_error_t *wifi_setup_enterprise(char *ssid, char *identity, char *username, char *password, unsigned char *cacert, int cacert_len, unsigned char *certificate, int certificate_len, unsigned char *privkey, int privkey_len, unsigned char *privpwd, int privpwd_len, int disabletimecheck, uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns1, uint32_t dns2, int powersave, int channel) {
    // TO DO
	#if 0
   driver_error_t *error;
    wifi_interface_t interface;
    tcpip_adapter_ip_info_t ip_info;
    ip_addr_t dns;
    ip_addr_t *dns_p = &dns;

    status_set(0x00000000, STATUS_WIFI_SETUP);

    if (status_get(STATUS_WIFI_INITED)) {
        wifi_mode_t curmode;
        if ((error = wifi_check_error(esp_wifi_get_mode(&curmode)))) return error;
        if (curmode == WIFI_MODE_AP) {
            //in case of switching mode AP<->STA Stop wifi
            if(status_get(STATUS_WIFI_STARTED)) {
                if ((error = wifi_check_error(esp_wifi_stop()))) return error;
                status_set(0x00000000, STATUS_WIFI_STARTED);
            }
            status_set(0x00000000, STATUS_WIFI_INITED);
        }
    }

    // Attach wifi driver
    if ((error = wifi_init(WIFI_MODE_STA))) return error;

    // Setup mode and config related to desired mode
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    strncpy((char *)wifi_config.sta.ssid, ssid, 32);
    wifi_config.sta.channel = (channel ? channel : 0);

    interface = ESP_IF_WIFI_STA;
    if ((error = wifi_check_error(esp_wifi_set_config(interface, &wifi_config)))) return error;

    if (powersave)
        if ((error = wifi_check_error(esp_wifi_set_ps(powersave)))) return error;

    if (disabletimecheck==0) {
        esp_wifi_sta_wpa2_ent_set_disable_time_check(false);
        WIFI_LOG("wpa2 enterprise certificate time check enabled\n");
    } else if (disabletimecheck==1) {
        esp_wifi_sta_wpa2_ent_set_disable_time_check(true);
        WIFI_LOG("wpa2 enterprise certificate time check disabled\n");
    }

    esp_wifi_sta_wpa2_ent_clear_ca_cert();
    if (cacert != NULL && cacert_len>0) {
        if ((error = wifi_check_error(esp_wifi_sta_wpa2_ent_set_ca_cert(cacert, cacert_len)))) return error;
        WIFI_LOG("wpa2 enterprise ca-cert set\n");
    }

    esp_wifi_sta_wpa2_ent_clear_cert_key();
    if (certificate != NULL && certificate_len>0 && privkey != NULL && privkey_len>0) {
        if ((error = wifi_check_error(esp_wifi_sta_wpa2_ent_set_cert_key(certificate, certificate_len, privkey, privkey_len, privpwd, privpwd_len)))) return error;
        if (privpwd_len > 0) {
            WIFI_LOG("wpa2 enterprise client certificate + key with password set\n");
        } else {
            WIFI_LOG("wpa2 enterprise client certificate + key set\n");
        }
    }

    esp_wifi_sta_wpa2_ent_clear_identity();
    if (identity != NULL && *identity != 0) {
        if ((error = wifi_check_error(esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)identity, strlen(identity))))) return error;
        WIFI_LOG("wpa2 enterprise identity set to: %s\n", identity);
    }

    esp_wifi_sta_wpa2_ent_clear_username();
    if (username != NULL && *username != 0) {
        if ((error = wifi_check_error(esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username))))) return error;
        WIFI_LOG("wpa2 enterprise user set to: %s\n", username);
    }
    esp_wifi_sta_wpa2_ent_clear_password();
    if (password != NULL && *password != 0) {
        if ((error = wifi_check_error(esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password))))) return error;
        WIFI_LOG("wpa2 enterprise password set\n");
    }

    esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
    if ((error = wifi_check_error(esp_wifi_sta_wpa2_ent_enable(&config)))) return error;
    WIFI_LOG("wpa2 enterprise enabled\n");

    status_set(STATUS_WIFI_SETUP, 0x00000000);

    // Set ip / mask / gw, if present
    if (ip && mask && gw) {
        ip_info.ip.addr = ip;
        ip_info.netmask.addr = mask;
        ip_info.gw.addr = gw;

        esp_netif_dhcpc_start(netif);
        tcpip_adapter_set_ip_info(ESP_IF_WIFI_STA, &ip_info);

        // If present, set dns1, else set to 8.8.8.8
        if (!dns1) dns1 = 134744072;
        ip_addr_set_ip4_u32(dns_p, dns1);

        dns_setserver(0, (const ip_addr_t *)&dns);

        // If present, set dns2, else set to 8.8.4.4
        if (!dns2) dns2 = 67373064;
        ip_addr_set_ip4_u32(dns_p, dns2);

        dns_setserver(1, (const ip_addr_t *)&dns);
    }
	#endif

    return NULL;
}

driver_error_t *wifi_start(uint8_t async) {
    driver_error_t *error;

    if (!async) {
        status_set(STATUS_WIFI_SYNC, 0x00000000);
    } else {
        status_set(0x00000000, STATUS_WIFI_SYNC);
    }

    if (!status_get(STATUS_WIFI_SETUP)) {
        return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_NOT_INIT, NULL);
    }

    if (!status_get(STATUS_WIFI_STARTED)) {
        if ((error = wifi_check_error(esp_wifi_start()))) return error;

        wifi_mode_t mode;
        if ((error = wifi_check_error(esp_wifi_get_mode(&mode)))) return error;

        if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
            status_set(STATUS_WIFI_STARTED, 0x00000000);
        } else {
            status_set(STATUS_WIFI_STARTED, 0x00000000);
            if (!async) {
                EventBits_t uxBits = xEventGroupWaitBits(netEvent, evWIFI_CONNECTED | evWIFI_CANT_CONNECT, pdTRUE, pdFALSE, portMAX_DELAY);
                if (uxBits & (evWIFI_CONNECTED)) {
                    if (!async) {
                        status_set(0x00000000, STATUS_WIFI_SYNC);
                    }
                    return NULL;
                }

                if (uxBits & (evWIFI_CANT_CONNECT)) {
                    esp_wifi_stop();
                    return driver_error(WIFI_DRIVER, WIFI_ERR_CANT_CONNECT, NULL);
                }
            }
        }
    }

    return NULL;
}

driver_error_t *wifi_stop() {
    driver_error_t *error;

    if (!status_get(STATUS_WIFI_SETUP)) {
        return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_NOT_INIT, NULL);
    }

    // TO DO
	#if 0
    esp_wifi_sta_wpa2_ent_disable();
	#endif

    // TO DO
	#if 0
    wifi_wps_disable();
	#endif

    // TO DO
	#if 0
    esp_smartconfig_stop();
	#endif

    if (status_get(STATUS_WIFI_STARTED)) {
        status_set(0x00000000, STATUS_WIFI_STARTED);

        if ((error = wifi_check_error(esp_wifi_stop()))) return error;
    }

    return NULL;
}

driver_error_t *wifi_stat(ifconfig_t *info) {
	esp_netif_ip_info_t ip_info;
    ip6_addr_t adr = {0};
    uint8_t mac[6] = {0,0,0,0,0,0};

    driver_error_t *error;

    uint8_t interface = ESP_IF_WIFI_STA;
    if (status_get(STATUS_WIFI_INITED)) {
        wifi_mode_t mode;
        if ((error = wifi_check_error(esp_wifi_get_mode(&mode)))) return error;

        if (mode == WIFI_MODE_AP)
            interface = ESP_IF_WIFI_AP;

        //TODO add stat for WIFI_MODE_APSTA
    }

    // Get WIFI IF info
    if ((error = wifi_check_error(esp_netif_get_ip_info(netif, &ip_info)))) return error;

    // TO DO
	#if 0
    if ((error = wifi_check_error(tcpip_adapter_get_ip6_linklocal(interface, &adr)))) ip6_addr_set(&adr,IP6_ADDR_ANY6);
	#endif

    // Get MAC info
    if (status_get(STATUS_WIFI_STARTED)) {
        if ((error = wifi_check_error(esp_wifi_get_mac(interface, mac)))) return error;
    }

    // Copy info
    info->gw.addr = ip_info.gw.addr;
    info->ip.addr = ip_info.ip.addr;
    info->netmask.addr = ip_info.netmask.addr;
    info->ip6 = adr;

    memcpy(info->mac, mac, sizeof(mac));

    return NULL;
}

driver_error_t *wifi_wps(int wpsmode, wifi_wps_pin_cb* callback) {
    // TO DO
	#if 0
    driver_error_t *error;

    status_set(0x00000000, STATUS_WIFI_SETUP);

    if (status_get(STATUS_WIFI_INITED)) {
        if(status_get(STATUS_WIFI_STARTED)) {
            if ((error = wifi_check_error(esp_wifi_stop()))) return error;
            status_set(0x00000000, STATUS_WIFI_STARTED);
        }
        status_set(0x00000000, STATUS_WIFI_INITED);
    }

    wps_mode = wpsmode;
    wps_pin_callback = callback;

    // Attach wifi driver
    if ((error = wifi_init(WIFI_MODE_STA))) return error; //does NOT work with APSTA
    status_set(STATUS_WIFI_SETUP, 0x00000000);

    if ((error = wifi_check_error(esp_wifi_start()))) return error;
    status_set(STATUS_WIFI_STARTED, 0x00000000);

    if (wps_mode != WPS_TYPE_DISABLE) {
        if (!wps_config) {
          wps_config = malloc(sizeof(esp_wps_config_t));
          if (!wps_config) return driver_error(WIFI_DRIVER, WIFI_ERR_WIFI_NO_MEM,NULL);
        }

        if (wps_mode == WPS_TYPE_PIN) {
          esp_wps_config_t wps_config_pin = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PIN);
          memcpy(wps_config, &wps_config_pin, sizeof(esp_wps_config_t));
        } else {
          esp_wps_config_t wps_config_pbc = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PBC);
          memcpy(wps_config, &wps_config_pbc, sizeof(esp_wps_config_t));
        }

        if ((error = wifi_check_error(esp_wifi_wps_enable(wps_config)))) return error;
        if ((error = wifi_check_error(esp_wifi_wps_start(0)))) return error;
    }
	#endif

    return NULL;
}

void wifi_wps_reconnect() {
    // TO DO
	#if 0
    esp_wifi_wps_disable();
    if (wps_mode != WPS_TYPE_DISABLE) {
        esp_wifi_wps_enable(wps_config);
        esp_wifi_wps_start(0);
    }
	#endif
}

void wifi_wps_disable() {
    // TO DO
	#if 0
    if (wps_mode != WPS_TYPE_DISABLE) {
        esp_wifi_wps_disable();
        wps_mode = WPS_TYPE_DISABLE;
    }
    if (wps_config) {
      free(wps_config);
      wps_config = NULL;
    }
	#endif
}

// TO DO
#if 0
static void* wifi_wps_pin_call(void* pin) {
    (*(wps_pin_callback))((char*)pin);
    return 0;
}
#endif

// TO DO
#if 0
void wifi_wps_pin(uint8_t *pin_code) {
    char *pin = (char*)malloc(9);
    snprintf(pin, 9, "%c%c%c%c%c%c%c%c", WPSPIN2STR(pin_code));
    pin[8] = '\0';
    if (wps_pin_callback != NULL) {
        pthread_t thread = 0;
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&thread, &attr, wifi_wps_pin_call, pin) != 0)
            thread = 0;
        pthread_setname_np(thread, "wifi_wps_pin");
        pthread_attr_destroy(&attr);
    }
}
#endif

// TO DO
#if 0
static void wifi_smartconfig_callback(smartconfig_status_t status, void *pdata)
{
    switch (status) {
        case SC_STATUS_WAIT:
            break;
        case SC_STATUS_FIND_CHANNEL:
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            /* may check for the type of smartconfig here
            {
                smartconfig_type_t *type = pdata;
                if (*type == SC_TYPE_ESPTOUCH) {
                    printf("SC_TYPE: ESPTOUCH\n");
                } else {
                    printf("SC_TYPE: AIRKISS\n");
                }
            }
            */
            break;
        case SC_STATUS_LINK:
            {
                wifi_config_t *wifi_config = pdata;
                esp_wifi_disconnect();

                // must connect to the new AP here to finish smartconnect
                esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config);
                esp_wifi_connect();

                if (wps_sc_callback != NULL) {
                    (*(wps_sc_callback))((char*)wifi_config->sta.ssid, (char*)wifi_config->sta.password);
                }
            }
            break;
        case SC_STATUS_LINK_OVER:
            if (pdata != NULL) {
                uint8_t phone_ip[4] = { 0 };
                memcpy(phone_ip, (uint8_t* )pdata, 4);
                printf("Remote IP: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
            }
            esp_smartconfig_stop();
            break;
        default:
            break;
    }
}
#endif

driver_error_t *wifi_smartconfig(wifi_sc_cb* callback) {
    // TO DO
	#if 0
    driver_error_t *error;
    wifi_mode_t mode = WIFI_MODE_STA;

    status_set(0x00000000, STATUS_WIFI_SETUP);

    if (status_get(STATUS_WIFI_INITED)) {
        if ((error = wifi_check_error(esp_wifi_get_mode(&mode)))) return error;

        if(status_get(STATUS_WIFI_STARTED)) {
            if ((error = wifi_check_error(esp_wifi_stop()))) return error;
            status_set(0x00000000, STATUS_WIFI_STARTED);
        }
        status_set(0x00000000, STATUS_WIFI_INITED);
    }

    wps_sc_callback = callback;

    // cannot use smartconfig in AP-only mode
    if (mode == WIFI_MODE_AP) {
        mode = WIFI_MODE_APSTA;
    }

    // Attach wifi driver
    if ((error = wifi_init(mode))) return error; //APSTA confirmed to work
    status_set(STATUS_WIFI_SETUP, 0x00000000);

    // make smartconfig restartable
    esp_smartconfig_stop();

    if ((error = wifi_check_error(esp_wifi_start()))) return error;
    status_set(STATUS_WIFI_STARTED, 0x00000000);

    //delay until wifi has been started...
    delay(10);

    //make sure we're not connected
    esp_wifi_disconnect();

    if ((error = wifi_check_error(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS)))) return error;
    if ((error = wifi_check_error(esp_smartconfig_start(wifi_smartconfig_callback, 0)))) return error;
	#endif

    return NULL;
}

driver_error_t *wifi_get_mac(uint8_t mac[6]) {
    driver_error_t *error;

    uint8_t interface = ESP_IF_WIFI_STA;
    if (status_get(STATUS_WIFI_INITED)) {
        wifi_mode_t mode;
        if ((error = wifi_check_error(esp_wifi_get_mode(&mode)))) return error;

        if (mode == WIFI_MODE_AP)
            interface = ESP_IF_WIFI_AP;

        //TODO add mac for WIFI_MODE_APSTA
    }

    // Get MAC info
    if (status_get(STATUS_WIFI_STARTED)) {
        if ((error = wifi_check_error(esp_wifi_get_mac(interface, mac)))) return error;
    }

    return NULL;
}

#endif

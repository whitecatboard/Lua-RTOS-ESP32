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
#include "esp_event_loop.h"
#include "esp_phy_init.h"
#include "tcpip_adapter.h"

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
#include <esp_wps.h>
#include <esp_smartconfig.h>
#include <pthread.h>

#define WIFI_LOG(m) syslog(LOG_DEBUG, m);

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

#define evWIFI_SCAN_END              ( 1 << 0 )
#define evWIFI_CONNECTED             ( 1 << 1 )
#define evWIFI_CANT_CONNECT          ( 1 << 2 )

static int wps_mode = WPS_TYPE_DISABLE;
static esp_wps_config_t wps_config_pbc = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PBC);
static esp_wps_config_t wps_config_pin = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PIN);
static wifi_wps_pin_cb* wps_pin_callback = NULL;
static wifi_sc_cb* wps_sc_callback = NULL;

#define WPSPIN2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5], (a)[6], (a)[7]

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

    if ((error = net_init())) {
        return error;
    }

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
        /* when scan()'ing shortly after esp_wifi_start() the result list may wrongly be empty
           so we waste some time here to make sure the system had some time to find some APs */
        delay(10);
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
    tcpip_adapter_ip_info_t ip_info;
    ip_addr_t dns;
    ip_addr_t *dns_p = &dns;

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

    if (mode == WIFI_MODE_STA) {
        // Setup mode and config related to desired mode
        wifi_config_t wifi_config;
        memset(&wifi_config, 0, sizeof(wifi_config_t));

        strncpy((char *)wifi_config.sta.ssid, ssid, 32);
        strncpy((char *)wifi_config.sta.password, password, 64);

        wifi_config.sta.channel = (channel ? channel : 0);

        interface = ESP_IF_WIFI_STA;
        if ((error = wifi_check_error(esp_wifi_set_config(interface, &wifi_config)))) return error;
    }
    if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
        // Setup mode and config related to desired mode
        wifi_config_t wifi_config;
        memset(&wifi_config, 0, sizeof(wifi_config_t));

        strncpy((char *)wifi_config.ap.ssid, ssid, 32);
        strncpy((char *)wifi_config.ap.password, password, 64);

        wifi_config.ap.ssid_len = 0;
        wifi_config.ap.channel = (channel ? channel : 1);
        wifi_config.ap.authmode = (*password ? WIFI_AUTH_WPA_WPA2_PSK : WIFI_AUTH_OPEN);
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
        ip_info.ip.addr = ip;
        ip_info.netmask.addr = mask;
        ip_info.gw.addr = gw;

        tcpip_adapter_dhcpc_stop(ESP_IF_WIFI_STA);
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

    wifi_wps_disable();
    esp_smartconfig_stop();

    if (status_get(STATUS_WIFI_STARTED)) {
        status_set(0x00000000, STATUS_WIFI_STARTED);

        if ((error = wifi_check_error(esp_wifi_stop()))) return error;
    }

    return NULL;
}

driver_error_t *wifi_stat(ifconfig_t *info) {
    tcpip_adapter_ip_info_t esp_info;
    ip6_addr_t adr;
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
    if ((error = wifi_check_error(tcpip_adapter_get_ip_info(interface, &esp_info)))) return error;

    if ((error = wifi_check_error(tcpip_adapter_get_ip6_linklocal(interface, &adr)))) ip6_addr_set(&adr,IP6_ADDR_ANY6);

    // Get MAC info
    if (status_get(STATUS_WIFI_STARTED)) {
        if ((error = wifi_check_error(esp_wifi_get_mac(interface, mac)))) return error;
    }

    // Copy info
    info->gw = esp_info.gw;
    info->ip = esp_info.ip;
    info->netmask = esp_info.netmask;
    info->ip6 = adr;

    memcpy(info->mac, mac, sizeof(mac));

    return NULL;
}

driver_error_t *wifi_wps(int wpsmode, wifi_wps_pin_cb* callback) {
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
        if ((error = wifi_check_error(esp_wifi_wps_enable(wps_mode == WPS_TYPE_PIN ? &wps_config_pin : &wps_config_pbc)))) return error;
        if ((error = wifi_check_error(esp_wifi_wps_start(0)))) return error;
    }
    return NULL;
}

void wifi_wps_reconnect() {
    esp_wifi_wps_disable();
    if (wps_mode != WPS_TYPE_DISABLE) {
        esp_wifi_wps_enable(wps_mode == WPS_TYPE_PIN ? &wps_config_pin : &wps_config_pbc);
        esp_wifi_wps_start(0);
    }
}

void wifi_wps_disable() {
    if (wps_mode != WPS_TYPE_DISABLE) {
        esp_wifi_wps_disable();
        wps_mode = WPS_TYPE_DISABLE;
    }
}

static void* wifi_wps_pin_call(void* pin) {
    (*(wps_pin_callback))((char*)pin);
    return 0;
}

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

driver_error_t *wifi_smartconfig(wifi_sc_cb* callback) {
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

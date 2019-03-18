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

#ifndef WIFI_H_
#define WIFI_H_

#include "sdkconfig.h"

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET

#include "net.h"

#include "esp_wifi.h"
#include "tcpip_adapter.h"

#include <sys/driver.h>

#define WIFI_CONNECT_RETRIES 2

// WIFI errors
#define WIFI_ERR_CANT_INIT              (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  0)
#define WIFI_ERR_CANT_CONNECT           (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  1)
#define WIFI_ERR_WIFI_FAIL              (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  2)
#define WIFI_ERR_WIFI_NO_MEM            (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  3)
#define WIFI_ERR_WIFI_NOT_INIT          (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  4)
#define WIFI_ERR_WIFI_NOT_START         (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  5)
#define WIFI_ERR_WIFI_IF                (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  6)
#define WIFI_ERR_WIFI_MODE              (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  7)
#define WIFI_ERR_WIFI_STATE             (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  8)
#define WIFI_ERR_WIFI_CONN              (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  9)
#define WIFI_ERR_WIFI_NVS               (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  10)
#define WIFI_ERR_WIFI_MAC               (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  11)
#define WIFI_ERR_WIFI_SSID              (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  12)
#define WIFI_ERR_WIFI_PASSWORD          (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  13)
#define WIFI_ERR_WIFI_TIMEOUT           (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  14)
#define WIFI_ERR_WAKE_FAIL              (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  15)
#define WIFI_ERR_INVALID_ARGUMENT       (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  16)
#define WIFI_ERR_NOT_SUPPORT            (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  17)
#define WIFI_ERR_NOT_STOPPED            (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  18)

extern const int wifi_errors;
extern const int wifi_error_map;

driver_error_t *wifi_scan(uint16_t *count, wifi_ap_record_t **list);
driver_error_t *wifi_setup(wifi_mode_t mode, char *ssid, char *password, uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns1, uint32_t dns2, int powersave, int channel, int hidden);
driver_error_t *wifi_setup_enterprise(char *ssid, char *identity, char *username, char *password, unsigned char *cacert, int cacert_len, unsigned char *certificate, int certificate_len, unsigned char *privkey, int privkey_len, unsigned char *privpwd, int privpwd_len, int disabletimecheck, uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns1, uint32_t dns2, int powersave, int channel);
driver_error_t *wifi_start(uint8_t async);
driver_error_t *wifi_stop();
driver_error_t *wifi_stat(ifconfig_t *info);
driver_error_t *wifi_get_mac(uint8_t mac[6]);

void wifi_wps_reconnect();
void wifi_wps_disable();
void wifi_wps_pin(uint8_t *pin_code);
typedef void wifi_wps_pin_cb(char* pin);
driver_error_t *wifi_wps(int wpsmode, wifi_wps_pin_cb* callback);

typedef void wifi_sc_cb(char* ssid, char* password);
driver_error_t *wifi_smartconfig(wifi_sc_cb* callback);

#endif

#endif

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

#ifndef WIFI_H_
#define WIFI_H_

#include "sdkconfig.h"

#if CONFIG_WIFI_ENABLED

#include "net.h"

#include "esp_wifi.h"
#include "tcpip_adapter.h"

#include <sys/driver.h>

#define WIFI_CONNECT_RETRIES 1

// WIFI errors
#define WIFI_ERR_CANT_INIT              (DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  0)
#define WIFI_ERR_CANT_CONNECT			(DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  1)
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
#define WIFI_ERR_INVALID_ARGUMENT		(DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  16)
#define WIFI_ERR_NOT_SUPPORT			(DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  17)
#define WIFI_ERR_NOT_STOPPED			(DRIVER_EXCEPTION_BASE(WIFI_DRIVER_ID) |  18)

driver_error_t *wifi_scan(uint16_t *count, wifi_ap_record_t **list);
driver_error_t *wifi_setup(wifi_mode_t mode, char *ssid, char *password, int powersave, int channel, int hidden);
driver_error_t *wifi_start();
driver_error_t *wifi_stop();
driver_error_t *wifi_stat(ifconfig_t *info);

#endif

#endif

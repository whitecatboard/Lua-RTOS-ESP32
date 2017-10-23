/*
 * Lua RTOS, simple channel LoRa WAN gateway
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

#ifndef LORA_GATEWAY_SINGLE_CHANNEL_GATEWAY_H_
#define LORA_GATEWAY_SINGLE_CHANNEL_GATEWAY_H_

#include "sx1276.h"

#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/spi.h>
#include <drivers/gpio.h>

#define LORA_DRIVER driver_get_by_name("lora")

// Lora errors
#define LORA_ERR_CANT_SETUP				            (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) |  0)
#define LORA_ERR_NO_MEM					            (DRIVER_EXCEPTION_BASE(LORA_DRIVER_ID) |  1)

extern const int lora_errors;
extern const int lora_error_map;

driver_error_t *lora_gw_setup(int band, const char *host, int port);
void lora_gw_unsetup();

#endif

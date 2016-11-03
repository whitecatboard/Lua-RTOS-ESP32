/*
 * Lua RTOS, Lora WAN driver for RN2483
 *
 * Copyright (C) 2015 - 2016
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

#include "whitecat.h"

#include <sys/drivers/error.h>

#ifndef LORA_H
#define LORA_H

#if LUA_USE_LORA
#define LORA_OK                                  (1 <<  0)
#define LORA_JOIN_ACCEPTED                       (1 <<  2)
#define LORA_TX_OK                               (1 <<  3)
#define LORA_RX_OK                               (1 <<  4)
#define LORA_OTHER                               (1 <<  5)
#define LORA_KEYS_NOT_CONFIGURED                 (1 <<  6)
#define LORA_ALL_CHANNELS_BUSY                   (1 <<  7)
#define LORA_DEVICE_IN_SILENT_STATE              (1 <<  8)
#define LORA_DEVICE_DEVICE_IS_NOT_IDLE           (1 <<  9)
#define LORA_PAUSED                              (1 << 10)
#define LORA_TIMEOUT                             (1 << 11)
#define LORA_JOIN_DENIED                         (1 << 12)
#define LORA_UNEXPECTED_RESPONSE                 (1 << 13)
#define LORA_NOT_JOINED                          (1 << 14)
#define LORA_REJOIN_NEEDED                       (1 << 15)
#define LORA_INVALID_DATA_LEN                    (1 << 16)
#define LORA_TRANSMISSION_FAIL_ACK_NOT_RECEIVED  (1 << 18)
#define LORA_NOT_SETUP                           (1 << 19)
#define LORA_INVALID_PARAM                       (1 << 20)
#define LORA_INVALID_ARGUMENT                    (1 << 20)
#define LORA_NO_MEM								 (1 << 21)

#define LORA_MAC_SET_DEVADDR		0
#define LORA_MAC_SET_DEVEUI			1
#define LORA_MAC_SET_APPEUI			2
#define LORA_MAC_SET_NWKSKEY		3
#define LORA_MAC_SET_APPSKEY		4
#define LORA_MAC_SET_APPKEY			5
#define LORA_MAC_SET_DR				6
#define LORA_MAC_SET_ADR			7
#define LORA_MAC_SET_RETX			8
#define LORA_MAC_SET_AR				9
#define LORA_MAC_SET_LINKCHK	   10
#define LORA_MAC_SET_CH_STATUS	   11
#define LORA_MAC_SET_CH_FREQ	   12
#define LORA_MAC_SET_CH_DCYCLE	   13
#define LORA_MAC_SET_CH_DRRANGE	   14
#define LORA_MAC_SET_PWRIDX		   15
	
#define LORA_MAC_GET_DEVADDR	   50
#define LORA_MAC_GET_DEVEUI		   51
#define LORA_MAC_GET_APPEUI		   52
#define LORA_MAC_GET_DR			   53
#define LORA_MAC_GET_ADR		   54
#define LORA_MAC_GET_RETX		   55
#define LORA_MAC_GET_AR			   56
#define LORA_MAC_GET_MRGN		   57
#define LORA_MAC_GET_CH_STATUS	   58
#define LORA_MAC_GET_CH_FREQ	   59
#define LORA_MAC_GET_CH_DCYCLE	   60
#define LORA_MAC_GET_CH_SDRANGE	   61
#define LORA_MAC_GET_PWRIDX		   62

	
typedef void (lora_rx)(int port, char *payload);

tdriver_error *lora_setup(int band);
int lora_mac_set(const char command, const char *value);
int lora_sys(const char *command, const char *value);
char *lora_sys_get(const char *command);
char *lora_mac_get(const char command);
int lora_join_otaa();
int lora_tx(int cnf, int port, const char *data);
void lora_set_rx_callback(lora_rx *callback);
int lora_mac(const char *command, const char *value);
tdriver_error *lora_reset();

#if USE_LMIC
#include <stdint.h>

tdriver_error *lmic_setup_timer();
void lmic_intr(u8_t irq);
#endif

#endif

#endif /* LORA_H */


/*
 * Lua RTOS, Lora WAN driver for LMIC
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

#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276 || CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_attr.h"

#include "lora.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/syslog.h>
#include <sys/mutex.h>
#include <sys/resource.h>
#include <sys/driver.h>
#include <sys/status.h>

#include "lmic.h"
#include "hex_string.h"

// Register driver and messages
void _lora_init();

DRIVER_REGISTER_BEGIN(LORA,lora, 0,_lora_init,NULL);
	DRIVER_REGISTER_ERROR(LORA, lora, KeysNotConfigured, "keys are not configured", LORA_ERR_KEYS_NOT_CONFIGURED);
	DRIVER_REGISTER_ERROR(LORA, lora, JoinDenied, "join denied", LORA_ERR_JOIN_DENIED);
	DRIVER_REGISTER_ERROR(LORA, lora, UnexpectedResponse, "unexpected response", LORA_ERR_UNEXPECTED_RESPONSE);
	DRIVER_REGISTER_ERROR(LORA, lora, NotJoined, "not joined", LORA_ERR_NOT_JOINED);
	DRIVER_REGISTER_ERROR(LORA, lora, NotSetup, "not setup", LORA_ERR_NOT_SETUP);
	DRIVER_REGISTER_ERROR(LORA, lora, NotEnoughtMemory, "not enough memory", LORA_ERR_NO_MEM);
	DRIVER_REGISTER_ERROR(LORA, lora, CannotSetup, "can't setup", LORA_ERR_CANT_SETUP);
	DRIVER_REGISTER_ERROR(LORA, lora, TransmissionFail, "transmission fail ack not received", LORA_ERR_TRANSMISSION_FAIL_ACK_NOT_RECEIVED);
	DRIVER_REGISTER_ERROR(LORA, lora, InvalidArgument, "invalid argument", LORA_ERR_INVALID_ARGUMENT);
	DRIVER_REGISTER_ERROR(LORA, lora, InvalidDataRate, "invalid data rate for your location", LORA_ERR_INVALID_DR);
	DRIVER_REGISTER_ERROR(LORA, lora, InvalidBand, "invalid band for your location", LORA_ERR_INVALID_BAND);
	DRIVER_REGISTER_ERROR(LORA, lora, NotAllowed, "not allowed", LORA_ERR_NOT_ALLOWED);
    DRIVER_REGISTER_ERROR(LORA, lora, InvalidFreq, "invalid frequency for your location", LORA_ERR_INVALID_FREQ);
DRIVER_REGISTER_END(LORA,lora, 0,_lora_init,NULL);

#define evLORA_INITED 	       	 ( 1 << 0 )
#define evLORA_JOINED  	       	 ( 1 << 1 )
#define evLORA_JOIN_DENIED     	 ( 1 << 2 )
#define evLORA_TX_COMPLETE    	 ( 1 << 3 )
#define evLORA_ACK_NOT_RECEIVED  ( 1 << 4 )

extern uint8_t flash_unique_id[8];

// LMIC job for start LMIC stack
static osjob_t initjob;

// Mutext for lora 
static struct mtx lora_mtx;

// Event group handler for sync LMIC events with driver functions
static EventGroupHandle_t loraEvent;

// Data required for OTAA
static u1_t APPEUI[8]  = {0,0,0,0,0,0,0,0};
static u1_t DEVEUI[8]  = {0,0,0,0,0,0,0,0};
static u1_t APPKEY[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

// This macro is used to check if data required for OTAA activation is available
#define have_otta_data() \
    ( \
        (memcmp(APPEUI, (u1_t[]){0,0,0,0,0,0,0,0}, 8) != 0) && \
        (memcmp(DEVEUI, (u1_t[]){0,0,0,0,0,0,0,0}, 8) != 0) && \
        (memcmp(APPKEY, (u1_t[]){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 16) != 0) \
    )

// Session data
RTC_DATA_ATTR static u4_t DEVADDR = 0x00000000;
RTC_DATA_ATTR static u1_t NWKSKEY[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
RTC_DATA_ATTR static u1_t APPSKEY[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

// This macro is used to check if session data to participate in a LoRa network
// is available
#define have_session_data() \
    ( \
        (DEVADDR != 0) && \
        (memcmp(NWKSKEY, (u1_t[]){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 16) != 0) && \
        (memcmp(APPSKEY, (u1_t[]){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 16) != 0) \
    )

static u1_t session_init = 0;

// Current message id. We put this in RTC memory for survive a deep sleep.
// ABP needs to keep msgid in sequence between tranfers.
RTC_DATA_ATTR static u4_t msgid = 0;

// If = 1 driver is setup, if = 0 is not setup
static u1_t setup = 0;

// Callback function to call when data is received
static lora_rx *lora_rx_callback = NULL;

// Table for translate numeric datarates to LMIC definitions
#if CONFIG_LUA_RTOS_LORA_BAND_EU868
static const u1_t data_rates[] = {
	DR_SF12, DR_SF11, DR_SF10, DR_SF9, DR_SF8, DR_SF7, DR_SF7B, DR_FSK, DR_NONE,
	DR_NONE, DR_NONE, DR_NONE, DR_NONE, DR_NONE, DR_NONE, DR_NONE
};
#endif

#if CONFIG_LUA_RTOS_LORA_BAND_US915
static const u1_t data_rates[] = {
	DR_SF10, DR_SF9, DR_SF8, DR_SF7, DR_SF8C, DR_NONE, DR_NONE, DR_NONE,
	DR_SF12CR, DR_SF11CR, DR_SF10CR, DR_SF9CR, DR_SF8CR, DR_SF7CR, DR_NONE,
	DR_NONE
};

#endif

// Current datarate set by user
static u1_t current_dr = 0;

// ADR active?
static u1_t adr = 0;

// LMIC event handler
void onEvent (ev_t ev) {
    switch(ev) {
	    case EV_SCAN_TIMEOUT:
	      break;

	    case EV_BEACON_FOUND:
	      break;

	    case EV_BEACON_MISSED:
	      break;

	    case EV_BEACON_TRACKED:
	      break;

	    case EV_JOINING:
	      break;

	    case EV_JOINED:
	      session_init = 1;

	      DEVADDR = LMIC.devaddr;
	      memcpy(NWKSKEY, LMIC.nwkKey, 16);
          memcpy(APPSKEY, LMIC.artKey, 16);

		  xEventGroupSetBits(loraEvent, evLORA_JOINED);
	      break;

	    case EV_RFU1:
	      break;

	    case EV_JOIN_FAILED:
	      session_init = 0;
		  xEventGroupSetBits(loraEvent, evLORA_JOIN_DENIED);
	      break;

	    case EV_REJOIN_FAILED:
	      session_init = 0;
	      break;

	    case EV_TXCOMPLETE:
		  if (LMIC.pendTxConf) {
			  if (LMIC.txrxFlags & TXRX_ACK) {
		  		  xEventGroupSetBits(loraEvent, evLORA_TX_COMPLETE);
			  }

			  if (LMIC.txrxFlags & TXRX_NACK) {
		  		  xEventGroupSetBits(loraEvent, evLORA_ACK_NOT_RECEIVED);
			  }
		  } else {
		      if (LMIC.dataLen && lora_rx_callback) {
				  // Make a copy of the payload and call callback function
				  u1_t *payload = (u1_t *)malloc(LMIC.dataLen * 2 + 1);
				  if (payload) {
					  // Coding payload into an hex string
					  val_to_hex_string((char *)payload, (char *)&LMIC.frame[LMIC.dataBeg], LMIC.dataLen, 0);
					  payload[LMIC.dataLen * 2] = 0x00;

					  lora_rx_callback(1, (char *)payload);
				  }
		      }

		      xEventGroupSetBits(loraEvent, evLORA_TX_COMPLETE);
		  }

	      break;

	    case EV_LOST_TSYNC:
	      break;

	    case EV_RESET:
	      break;

	    case EV_RXCOMPLETE:
	      break;

	    case EV_LINK_DEAD:
	      break;

	    case EV_LINK_ALIVE:
	      break;

	    default:
	      break;
  	}
}

// LMIC first job
static void lora_init(osjob_t *j) {
    // Reset MAC state
    LMIC_reset();

    // Add channels
	#if CONFIG_LUA_RTOS_LORA_BAND_EU868
        LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7 ), BAND_CENTI);
	    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);
	    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7 ), BAND_CENTI);
	    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7 ), BAND_CENTI);
	    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7 ), BAND_CENTI);
	    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7 ), BAND_CENTI);
	    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7 ), BAND_CENTI);
	    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7 ), BAND_CENTI);
	    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK ), BAND_MILLI);
	#endif

	#if CONFIG_LUA_RTOS_LORA_BAND_US915
	    LMIC_selectSubBand(1);
	#endif

	// Disable link check validation
    LMIC_setLinkCheckMode(0);

    // ADR disabled
    adr = 0;
    LMIC_setAdrMode(0);

    if (have_session_data()) {
        // If at this point session data is available, means that CPU has been waked up from
        // deep sleep, were session data is stored into RTC memory
        syslog(LOG_DEBUG, "lora: restore session from RTC");
        LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
        session_init = 1;
    }

    // TTN uses SF9 for its RX2 window
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    current_dr = DR_SF7;
    LMIC_setDrTxpow(current_dr, 14);

	// Inform waiting thread that stack is initialized
    xEventGroupSetBits(loraEvent, evLORA_INITED);
}

// Setup driver
driver_error_t *lora_setup(int band) {
	#if CONFIG_LUA_RTOS_LORA_BAND_EU868
	if (band != 868) {
		return driver_error(LORA_DRIVER, LORA_ERR_INVALID_BAND, NULL);
	}
	#endif

	#if CONFIG_LUA_RTOS_LORA_BAND_US915
	if (band != 915) {
		return driver_error(LORA_DRIVER, LORA_ERR_INVALID_BAND, NULL);
	}
	#endif

    mtx_lock(&lora_mtx);

    if (!setup) {
        syslog(LOG_DEBUG, "lora: setup, band %d", band);
		
		// LMIC init
        driver_error_t *error;

		if (!(error = os_init())) {
	        // Create event group for sync driver with LMIC events
			loraEvent = xEventGroupCreate();

			// Set first callback, for init lora stack
			os_setCallback(&initjob, lora_init);

			// Wait for stack initialization
		    xEventGroupWaitBits(loraEvent, evLORA_INITED, pdTRUE, pdFALSE, portMAX_DELAY);
		} else {
			setup = 0;

			mtx_unlock(&lora_mtx);

    		return error;
		}

    }

	setup = 1;

    mtx_unlock(&lora_mtx);
    
    return NULL;
}

driver_error_t *lora_mac_set(const char command, const char *value) {
    mtx_lock(&lora_mtx);

    if (!setup) {
        mtx_unlock(&lora_mtx);
		return driver_error(LORA_DRIVER, LORA_ERR_NOT_SETUP, NULL);
    }

	switch(command) {
		case LORA_MAC_SET_DEVADDR:
			hex_string_to_val((char *)value, (char *)(&DEVADDR), 4, 1);
			break;
		
		case LORA_MAC_SET_DEVEUI:
			#if CONFIG_LUA_RTOS_READ_FLASH_UNIQUE_ID
			mtx_unlock(&lora_mtx);
			return driver_error(LORA_DRIVER, LORA_ERR_INVALID_ARGUMENT, "in this board DevEui is assigned automatically");
			#else
			// DEVEUI must be in little-endian format
			hex_string_to_val((char *)value, (char *)DEVEUI, 8, 1);
			#endif
			break;
		
		case LORA_MAC_SET_APPEUI:
			// APPEUI must be in little-endian format
			hex_string_to_val((char *)value, (char *)APPEUI, 8, 1);
			break;
		
		case LORA_MAC_SET_NWKSKEY:
			hex_string_to_val((char *)value, (char *)NWKSKEY, 16, 0);
			break;
		
		case LORA_MAC_SET_APPSKEY:
			hex_string_to_val((char *)value, (char *)APPSKEY, 16, 0);
			break;
		
		case LORA_MAC_SET_APPKEY:
			// APPKEY must be in big-endian format
			hex_string_to_val((char *)value, (char *)APPKEY, 16, 0);
			break;
		
		case LORA_MAC_SET_DR:
			if ((atoi((char *)value) < 0) || (atoi((char *)value) > 15)) {
				return driver_error(LORA_DRIVER, LORA_ERR_INVALID_DR, NULL);
			}

			u1_t dr = data_rates[atoi((char *)value)];
			if (dr == DR_NONE) {
				return driver_error(LORA_DRIVER, LORA_ERR_INVALID_DR, NULL);
			}

			current_dr = dr;

			if (!adr) {
				LMIC_setDrTxpow(current_dr, 14);
			}

			break;
		
		case LORA_MAC_SET_ADR:
			if (strcmp(value, "on") == 0) {
				adr = 1;
				LMIC_setAdrMode(1);
			} else {
				adr = 0;
				LMIC_setAdrMode(0);
			}
			break;
		
		case LORA_MAC_SET_LINKCHK:
			if (strcmp(value, "on") == 0) {
				LMIC_setLinkCheckMode(1);
			} else {
				LMIC_setLinkCheckMode(0);
			}
			break;

		case LORA_MAC_SET_RETX:
			LMIC.txAttempts = atoi((char *)value);
			break;
	}

    mtx_unlock(&lora_mtx);

	return NULL;
}

driver_error_t *lora_mac_get(const char command, char **value) {
	char *result = NULL;

    mtx_lock(&lora_mtx);

	switch(command) {
		case LORA_MAC_GET_DEVADDR:
            result = (char *)malloc(9);
            val_to_hex_string(result, (char *)(&DEVADDR), 4, 1);
			break;
		
		case LORA_MAC_GET_DEVEUI:
			result = (char *)malloc(17);
			
			// DEVEUI is in little-endian format
			val_to_hex_string(result, (char *)DEVEUI, 8, 1);
			break;
		
		case LORA_MAC_GET_APPEUI:
			result = (char *)malloc(17);
			
			// APPEUI is in little-endian format
			val_to_hex_string(result, (char *)APPEUI, 8, 1);
			break;

		case LORA_MAC_GET_DR:
			result = (char *)malloc(2);
			if (result) {
				sprintf(result,"%d",current_dr);
			}
			break;
		
		case LORA_MAC_GET_ADR:
			if (LMIC.adrEnabled) {
				result = (char *)malloc(3);
				if (result) {
					strcpy(result, "on");
				}
			} else {
				result = (char *)malloc(4);
				if (result) {
					strcpy(result, "off");
				}				
			}
			break;

		case LORA_MAC_GET_LINKCHK:
			if (LMIC.adrAckReq == LINK_CHECK_INIT) {
				result = (char *)malloc(3);
				if (result) {
					strcpy(result, "on");
				}
			} else {
				result = (char *)malloc(4);
				if (result) {
					strcpy(result, "off");
				}
			}
			break;

		case LORA_MAC_GET_RETX:
			result = (char *)malloc(2);
			if (result) {
				sprintf(result,"%d",LMIC.txAttempts);
			}
			break;
	}

    mtx_unlock(&lora_mtx);

    *value = result;

	return NULL;
}

driver_error_t *lora_join() {
    mtx_lock(&lora_mtx);

    // Sanity checks
    if (!setup) {
        mtx_unlock(&lora_mtx);
		return driver_error(LORA_DRIVER, LORA_ERR_NOT_SETUP, NULL);
    }

    if (have_session_data()) {
        // Session data is available, this means that we have joined in the past,
        // or MCU has been waked up from deep sleep and session has been restored,
        // or node is activated by ABP and DevAddr, NWKSessionKey, APPSessionKey have
        // been set by the programmer.
        mtx_unlock(&lora_mtx);
        return NULL;
    }

    if (!have_otta_data()) {
        // We don't have data for OTAA activation
        mtx_unlock(&lora_mtx);
		return driver_error(LORA_DRIVER, LORA_ERR_KEYS_NOT_CONFIGURED, NULL);
    }

    // Set msgid to 0
    msgid = 0;

    // Set DR
    if (!adr) {
        LMIC_setDrTxpow(current_dr, 14);
    }

    // Do join
	hal_lmic_join();

	// Wait for one of the expected events
    EventBits_t uxBits = xEventGroupWaitBits(loraEvent, evLORA_JOINED | evLORA_JOIN_DENIED, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & (evLORA_JOINED)) {
	    mtx_unlock(&lora_mtx);   
		return NULL;
    }

    if (uxBits & (evLORA_JOIN_DENIED)) {
	    mtx_unlock(&lora_mtx);   
		return driver_error(LORA_DRIVER, LORA_ERR_NOT_JOINED, NULL);
    }
	
	mtx_unlock(&lora_mtx);

	return driver_error(LORA_DRIVER, LORA_ERR_UNEXPECTED_RESPONSE, NULL);
}

driver_error_t *lora_tx(int cnf, int port, const char *data) {
	uint8_t *payload;
	uint8_t payload_len;
	
    mtx_lock(&lora_mtx);

    if (!setup) {
        mtx_unlock(&lora_mtx);
        return driver_error(LORA_DRIVER, LORA_ERR_NOT_SETUP, NULL);
    }

    if (have_session_data()) {
        if (!session_init) {
            // Session data is available, so set session if it has not yet been done
            LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
            session_init = 1;
        }
    } else {
        // Session data is not available
        if (have_otta_data()) {
            // Not joined
            mtx_unlock(&lora_mtx);
            return driver_error(LORA_DRIVER, LORA_ERR_NOT_JOINED, NULL);
        } else {
            mtx_unlock(&lora_mtx);
            return driver_error(LORA_DRIVER, LORA_ERR_KEYS_NOT_CONFIGURED, NULL);
        }
    }

	payload_len = strlen(data) / 2;

	// Allocate buffer por payload	
	payload = (uint8_t *)malloc(payload_len + 1);
	if (!payload) {
		mtx_unlock(&lora_mtx);
		return driver_error(LORA_DRIVER, LORA_ERR_NO_MEM, NULL);
	}
	
	// Convert input payload (coded in hex string) into a byte buffer
	hex_string_to_val((char *)data, (char *)payload, payload_len, 0);

	// Put message id
	msgid++;

	LMIC.seqnoUp = msgid;
	payload[payload_len] = msgid;

	// Set DR
	if (!adr) {
		LMIC_setDrTxpow(current_dr, 14);
	}

	// Send
	hal_lmic_tx(port, payload, payload_len, cnf);

	// Wait for one of the expected events
    EventBits_t uxBits = xEventGroupWaitBits(loraEvent, evLORA_TX_COMPLETE | evLORA_ACK_NOT_RECEIVED, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & (evLORA_TX_COMPLETE)) {
	    mtx_unlock(&lora_mtx);   
		return NULL;
    }

    if (uxBits & (evLORA_ACK_NOT_RECEIVED)) {
        mtx_unlock(&lora_mtx);
        return driver_error(LORA_DRIVER, LORA_ERR_TRANSMISSION_FAIL_ACK_NOT_RECEIVED, NULL);
    }
	
	mtx_unlock(&lora_mtx);

    return driver_error(LORA_DRIVER, LORA_ERR_UNEXPECTED_RESPONSE, NULL);
}

void lora_set_rx_callback(lora_rx *callback) {
    mtx_lock(&lora_mtx);
	
    lora_rx_callback = callback;
    
	mtx_unlock(&lora_mtx);
}

// This functions are needed for the LMIC stack for pass
// connection data
void os_getArtEui (u1_t* buf) { 
	memcpy(buf, APPEUI, 8);
}

void os_getDevEui (u1_t* buf) {
	memcpy(buf, DEVEUI, 8);
}

void os_getDevKey (u1_t* buf) { 
	memcpy(buf, APPKEY, 16);
}

void _lora_init() {
    // Create lora mutex
    mtx_init(&lora_mtx, NULL, NULL, 0);

    // LMIC need to mantain some information in RTC
    status_set(STATUS_NEED_RTC_SLOW_MEM, 0x00000000);

    // Get device EUI from flash id
	#if CONFIG_LUA_RTOS_READ_FLASH_UNIQUE_ID
    	int i = 0;

    	for(i=0;i<8;i++) {
    		DEVEUI[i] = flash_unique_id[7-i];
    	}
	#endif
}

#endif

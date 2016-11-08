/*
 * Lua RTOS, Lora WAN driver for LMIC
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

#if LUA_USE_LORA
#if USE_LMIC

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "event_groups.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/syslog.h>
#include <sys/mutex.h>
#include <sys/drivers/resource.h>
#include <sys/drivers/lora.h>
 
#include "lmic.h"

#define LORA_DEBUG_LEVEL 0

#define evLORA_JOINED  	       	 ( 1 << 0 )
#define evLORA_JOIN_DENIED     	 ( 1 << 1 )
#define evLORA_TX_COMPLETE    	 ( 1 << 2 )
#define evLORA_ACK_NOT_RECEIVED  ( 1 << 3 )

// Mutext for lora 
static struct mtx lora_mtx;

// Event group handler for sync LMIC events with driver functions
static EventGroupHandle_t loraEvent;

// Current APPEU, DEVEUI, APPKEY ....
static u1_t APPEUI[8] = {0,0,0,0,0,0,0,0};
static u1_t DEVEUI[8] = {0,0,0,0,0,0,0,0};
static u1_t APPKEY[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

// Current message id
static u8_t msgid = 0;

// If = 1 driver is setup, if = 0 is not setup
static int setup = 0;

// Current used band
static int current_band = 868;

// Callback function to call when data is received
static lora_rx *lora_rx_callback = NULL;

// This function is called from mach_dev
void _lora_init() {
    // Create lora mutex
    mtx_init(&lora_mtx, NULL, NULL, 0);
}

// Convert a buffer coded into an hex string (hbuff) into a byte buffer (vbuff)
// Length of byte buffer is len
static void hex_string_to_val(char *hbuff, char *vbuff, int len, int rev) {
    int  i;
    char c;
	
	// If reverse, put hbuff at the last byte
	if (rev) {
		while(*hbuff) hbuff++;
		hbuff -= 2;
	}
    
    for(i=0;i<len;i++) {
        c = 0;

        if ((*hbuff >= '0') && (*hbuff <= '9')) {
            c = (0 + (*hbuff - '0')) << 4;
        }

        if ((*hbuff >= 'A') && (*hbuff <= 'F')) {
            c = (10 + (*hbuff - 'A')) << 4;
        }
        
        hbuff++;

        if ((*hbuff >= '0') && (*hbuff <= '9')) {
            c |= 0 + (*hbuff - '0');
        }

        if ((*hbuff >= 'A') && (*hbuff <= 'F')) {
            c |= 10 + (*hbuff - 'A');
        }
        
        *vbuff = c;
		
		if (rev) {
			hbuff -= 3;
		} else {
	        hbuff++;			
		}
		
        vbuff++;
    }
}

// Convert byte buffer (vbuff argument) of len argument size into a hex
// string buffer (hbuff argument) into a ) 
static void val_to_hex_string(char *hbuff, char *vbuff, int len, int reverse) {
    int i;
	
	if (reverse) {
		vbuff += (len - 1);
	}

    for(i=0;i<len;i++) {
        if ((((*vbuff & 0xf0) >> 4) >= 0) && (((*vbuff & 0xf0) >> 4) <= 9)) {
            *hbuff = '0' + ((*vbuff & 0xf0) >> 4);
        }
        
        if ((((*vbuff & 0xf0) >> 4) >= 10) && (((*vbuff & 0xf0) >> 4) <= 15)) {
            *hbuff = 'A' + (((*vbuff & 0xf0) >> 4) - 10);
        }
        hbuff++;

        if (((*vbuff & 0x0f) >= 0) && ((*vbuff & 0x0f) <= 9)) {
            *hbuff = '0' + (*vbuff & 0x0f);
        }
        
        if (((*vbuff & 0x0f) >= 10) && ((*vbuff & 0x0f) <= 15)) {
            *hbuff = 'A' + ((*vbuff & 0x0f) - 10);
        }
        hbuff++;  
		
		if (reverse) {
	        vbuff--;						
		} else {
	        vbuff++;			
		}
    }    
	
	*hbuff = 0x00;
}

// LMIC event handler
void onEvent (ev_t ev) {
    switch(ev) {
	    case EV_SCAN_TIMEOUT:
		  #if LORA_DEBUG_LEVEL > 0
	 	  printf("EV_SCAN_TIMEOUT\r\n");
          #endif
	      break;

	    case EV_BEACON_FOUND:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_BEACON_FOUND\r\n");
          #endif
	      break;

	    case EV_BEACON_MISSED:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_BEACON_MISSED\r\n");
          #endif
	      break;

	    case EV_BEACON_TRACKED:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_BEACON_TRACKED\r\n");
          #endif
	      break;

	    case EV_JOINING:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_JOINING\r\n");
          #endif
	      break;

	    case EV_JOINED:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_JOINED\r\n");
          #endif
		  xEventGroupSetBits(loraEvent, evLORA_JOINED);
	      break;

	    case EV_RFU1:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_RFU1\r\n");
          #endif
	      break;

	    case EV_JOIN_FAILED:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_JOIN_FAILED\r\n");
          #endif
		  xEventGroupSetBits(loraEvent, evLORA_JOIN_DENIED);
	      break;

	    case EV_REJOIN_FAILED:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_REJOIN_FAILED\r\n");
          #endif
	      break;

	    case EV_TXCOMPLETE:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_TXCOMPLETE\r\n");
          printf("LMIC.pendTxConf = %d, LMIC.txrxFlags = %x\r\n", LMIC.pendTxConf, LMIC.txrxFlags);
          printf("EV_TXCOMPLETE\r\n");
          #endif
		  if (LMIC.pendTxConf) {
			  if (LMIC.txrxFlags & TXRX_ACK) {
		  		  xEventGroupSetBits(loraEvent, evLORA_TX_COMPLETE);
			  }

			  if (LMIC.txrxFlags & TXRX_NACK) {
		  		  xEventGroupSetBits(loraEvent, evLORA_ACK_NOT_RECEIVED);
			  }
		  } else {
	  		  xEventGroupSetBits(loraEvent, evLORA_TX_COMPLETE);
		  }

	      if (LMIC.dataLen && lora_rx_callback) {
			  #if LORA_DEBUG_LEVEL > 0
			  printf("something received of length %d bytes\r\n", LMIC.dataLen * 2 + 1);
			  #endif

			  // Make a copy of the payload and call callback function
			  u8_t *payload = (u8_t *)malloc(LMIC.dataLen);
			  if (payload) {
				  #if LORA_DEBUG_LEVEL > 0
				  printf("call to lora_rx_callback\r\n");	
				  #endif
				  
				  // Coding payload into an hex string
				  val_to_hex_string((char *)payload, (char *)&LMIC.frame[LMIC.dataBeg], LMIC.dataLen, 0);
				  payload[LMIC.dataLen * 2] = 0x00;

				  lora_rx_callback(1, (char *)payload);
			  }
	      }
	      break;

	    case EV_LOST_TSYNC:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_LOST_TSYNC\r\n");
          #endif
	      break;

	    case EV_RESET:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_RESET\r\n");
          #endif
	      break;

	    case EV_RXCOMPLETE:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_RXCOMPLETE\r\n");
          #endif
	      break;

	    case EV_LINK_DEAD:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_LINK_DEAD\r\n");
          #endif
	      break;

	    case EV_LINK_ALIVE:
          #if LORA_DEBUG_LEVEL > 0
          printf("EV_LINK_ALIVE\r\n");
          #endif
	      break;

	    default:
	      break;
  	}
}

// LMIC first job
static void lora_init(osjob_t* j) {
    // Reset MAC state
    LMIC_reset();

	if (current_band == 868) {
	    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
	    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);
	    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
	    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
	    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
	    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
	    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
	    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
	    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);	
	}

    /* Disable link check validation */
    LMIC_setLinkCheckMode(0);

    /* adr disabled */
    LMIC_setAdrMode(0);

    /* TTN uses SF9 for its RX2 window. */
    LMIC.dn2Dr = DR_SF9;

    /* Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library) */
    LMIC_setDrTxpow(DR_SF12, 14);
}

// Setup driver
tdriver_error *lora_setup(int band) {
	syslog(LOG_DEBUG, "lora: setup, band %d", band);

    current_band = band;    

    if (!setup) {
		// Create event group for sync driver with LMIC events
		loraEvent = xEventGroupCreate();
		
		// LMIC init
		os_init();

		// Set first callback, for init lora stack
		osjob_t *initjob = (osjob_t *)malloc(sizeof(osjob_t));
		if (initjob) {
			os_setCallback(initjob, lora_init);
		}
    }
    
	setup = 1;

    return NULL;
}

int lora_mac(const char *command, const char *value) {	
	return LORA_OK;
}

int lora_sys(const char *command, const char *value) {
	return LORA_OK;
}

int lora_mac_set(const char command, const char *value) {
    mtx_lock(&lora_mtx);

    if (!setup) {
        mtx_unlock(&lora_mtx);
        return LORA_NOT_SETUP;
    }

	switch(command) {
		case LORA_MAC_SET_DEVADDR:
			break;
		
		case LORA_MAC_SET_DEVEUI:
			// DEVEUI must be in little-endian format
			hex_string_to_val((char *)value, (char *)DEVEUI, 8, 1);
			break;
		
		case LORA_MAC_SET_APPEUI:
			// APPEUI must be in little-endian format
			hex_string_to_val((char *)value, (char *)APPEUI, 8, 1);
			break;
		
		case LORA_MAC_SET_NWKSKEY:
			break;
		
		case LORA_MAC_SET_APPSKEY:
			break;
		
		case LORA_MAC_SET_APPKEY:
			// APPKEY must be in big-endian format
			hex_string_to_val((char *)value, (char *)APPKEY, 16, 0);
			break;
		
		case LORA_MAC_SET_DR:
			break;
		
		case LORA_MAC_SET_ADR:
			if (strcmp(value, "on")) {
				LMIC_setAdrMode(1);
			} else {
				LMIC_setAdrMode(0);
			}
			break;
		
		case LORA_MAC_SET_RETX:
			break;
		
		case LORA_MAC_SET_AR:
			break;
		
		case LORA_MAC_SET_LINKCHK:
			if (strcmp(value, "on")) {
				LMIC_setLinkCheckMode(1);
			} else {
				LMIC_setLinkCheckMode(0);
			}
			break;
	}

    mtx_unlock(&lora_mtx);

	return LORA_OK;
}

char *lora_mac_get(const char command) {
	char *result;

    mtx_lock(&lora_mtx);

    if (!setup) {
        mtx_unlock(&lora_mtx);
        return NULL;
    }
	
	switch(command) {
		case LORA_MAC_GET_DEVADDR:
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
		
		case LORA_MAC_GET_RETX:
			break;
		
		case LORA_MAC_GET_AR:
			break;
		
		case LORA_MAC_GET_MRGN:
			break;
	}

    mtx_unlock(&lora_mtx);

	return NULL;
}

char *lora_sys_get(const char *command) {
	return NULL;
}

int lora_join_otaa() {
    mtx_lock(&lora_mtx);

    if (!setup) {
        mtx_unlock(&lora_mtx);
        return LORA_NOT_SETUP;
    }

	// TODO: LORA_KEYS_NOT_CONFIGURED

	LMIC_startJoining();

	// Wait for one of the expected events
    EventBits_t uxBits = xEventGroupWaitBits(loraEvent, evLORA_JOINED | evLORA_JOIN_DENIED, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & (evLORA_JOINED)) {
	    mtx_unlock(&lora_mtx);   
		return LORA_JOIN_ACCEPTED; 	
    }

    if (uxBits & (evLORA_JOIN_DENIED)) {
	    mtx_unlock(&lora_mtx);   
		return LORA_NOT_JOINED; 	
    }
	
	mtx_unlock(&lora_mtx);
	return LORA_UNEXPECTED_RESPONSE;	
}

int lora_tx(int cnf, int port, const char *data) {
	uint8_t *payload;
	uint8_t payload_len;
	
    mtx_lock(&lora_mtx);

    if (!setup) {
        mtx_unlock(&lora_mtx);
        return LORA_NOT_SETUP;
    }
	
	payload_len = strlen(data) / 2;

	// Allocate buffer por payload	
	payload = (uint8_t *)malloc(payload_len + 1);
	if (!payload) {
		// TO DO: not memory
	}
	
	// Convert input payload (coded in hex string) into a byte buffer
	hex_string_to_val((char *)data, (char *)payload, payload_len, 0);

	// Put message id
	payload[payload_len] = msgid++;

	// Send 
    LMIC_setTxData2(port, payload, sizeof(payload), cnf);
		
	// Wait for one of the expected events
    EventBits_t uxBits = xEventGroupWaitBits(loraEvent, evLORA_TX_COMPLETE | evLORA_ACK_NOT_RECEIVED, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & (evLORA_TX_COMPLETE)) {
	    mtx_unlock(&lora_mtx);   
		return LORA_TX_OK; 	
    }

    if (uxBits & (evLORA_ACK_NOT_RECEIVED)) {
	    mtx_unlock(&lora_mtx);   
		return LORA_TRANSMISSION_FAIL_ACK_NOT_RECEIVED; 	
    }
	
    mtx_unlock(&lora_mtx);

	return LORA_UNEXPECTED_RESPONSE;
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

#endif
#endif

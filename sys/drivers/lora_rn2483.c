/*
 * Whitecat, Lora WAN driver for RN2483
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

#if LUA_USE_LORA
#if USE_RN2483

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/syslog.h>
#include <sys/mutex.h>

#include <sys/drivers/gpio.h>
#include <sys/drivers/resource.h>
#include <sys/drivers/uart.h>
#include <sys/drivers/lora.h>
#include <sys/drivers/console.h>
#include <sys/delay.h>
 
#define LORA_TIMER_FREQ         45000 / portTICK_PERIOD_MS

// Expected events after enter of sys command
#define LORA_SYS_ENTER_COMMAND  \
    (LORA_OK | LORA_OK)

// Expected events after enter of mac command
#define LORA_MAC_ENTER_COMMAND  \
    (LORA_OK)

// Expected events after enter of set command
#define LORA_SET_ENTER_COMMAND  \
    (LORA_OK)

// Expected events after enter of mac get command
#define LORA_GET_ENTER_COMMAND  \
    (LORA_OK)

// Expected events after enter of join command
#define LORA_JOIN_ENTER_COMMAND  \
    (LORA_OK | LORA_KEYS_NOT_CONFIGURED | LORA_ALL_CHANNELS_BUSY | \
     LORA_DEVICE_IN_SILENT_STATE | \
     LORA_PAUSED)

// Expected events after end of join command
#define LORA_JOIN_END_COMMAND  \
    (LORA_JOIN_ACCEPTED | LORA_JOIN_DENIED)

// Expected events after enter of tx command
#define LORA_TX_ENTER_COMMAND  \
    (LORA_OK | LORA_NOT_JOINED | LORA_ALL_CHANNELS_BUSY | \
     LORA_DEVICE_IN_SILENT_STATE | LORA_REJOIN_NEEDED | \
     LORA_DEVICE_DEVICE_IS_NOT_IDLE | LORA_PAUSED | LORA_INVALID_DATA_LEN)

// Expected events after end of tx command
#define LORA_TX_END_COMMAND  \
    (LORA_TX_OK | LORA_TRANSMISSION_FAIL_ACK_NOT_RECEIVED | \
     LORA_INVALID_DATA_LEN)

// Timer handle for Lora
static TimerHandle_t loraTimer = NULL;

static int joined = 0;       // Are joined?
static int otaa = 0;        // Las join were by OTAA?
static int uart_setup = 0; // Driver uart is setup?
static int setup = 0;       // Driver is setup?
static int current_band = 868;

// Callback function to call when data is received
static lora_rx *lora_rx_callback = NULL;

// Mutext for lora 
static struct mtx lora_mtx;

void _lora_init() {
    // Create lora mutex
    mtx_init(&lora_mtx, NULL, NULL, 0);
	
	// Configure reset pin, and put to the low state (module not available)
    gpio_pin_output(LORA_RST_PIN);
    gpio_pin_clr(LORA_RST_PIN);
}

// This function parses a string received over the UART, and transform it into
// a token
static int lora_parse_response(char *resp) {
    if (strcmp(resp, "ok") == 0) {
        return LORA_OK;
    } else if (strcmp(resp, "accepted") == 0) {    
        return LORA_JOIN_ACCEPTED;
    } else if (strcmp(resp, "denied") == 0) {    
        return LORA_JOIN_DENIED;
    } else if (strcmp(resp, "mac_tx_ok") == 0) {    
        return LORA_TX_OK;
    } else if (strncmp(resp, "mac_rx", 6) == 0) {    
        return LORA_RX_OK;
    } else if (strcmp(resp, "keys_not_init") == 0) {    
        return LORA_KEYS_NOT_CONFIGURED;
    } else if (strcmp(resp, "no_free_ch") == 0) {
        return LORA_ALL_CHANNELS_BUSY;
    } else if (strcmp(resp, "silent") == 0) {
        return LORA_DEVICE_IN_SILENT_STATE;
    } else if (strcmp(resp, "busy") == 0) {
        return LORA_DEVICE_DEVICE_IS_NOT_IDLE;
    } else if (strcmp(resp, "mac_paused") == 0) {
        return LORA_PAUSED;
    } else if (strcmp(resp, "not_joined") == 0) {
        return LORA_NOT_JOINED;
    } else if (strcmp(resp, "frame_counter_err_rejoin_needed") == 0) {
        return LORA_REJOIN_NEEDED;
    } else if (strcmp(resp, "invalid_data_len") == 0) {
        return LORA_INVALID_DATA_LEN;
    } else if (strcmp(resp, "mac_err") == 0) {
        return LORA_TRANSMISSION_FAIL_ACK_NOT_RECEIVED;
    } else if (strcmp(resp, "invalid_param") == 0) {
        return LORA_INVALID_PARAM;
    } else {
        return LORA_OTHER;
    }
}

static int lora_response(char *outBuffer, int timeout) {
    char buffer[255];
    int resp;
    
    if (uart_reads(LORA_UART, buffer, 1, timeout)) {
        syslog(LOG_DEBUG, "lora: %s", buffer);
        
        if ((resp = lora_parse_response(buffer)) == LORA_RX_OK) {
            // Something received
            char *payload;
    		int port;

            // Allocate space for the payload
            payload = (char *)malloc(255);
            if (!payload) {
            	return LORA_NO_MEM;
            }

            // Get received port / payload
            payload[0] = 0x00;
            sscanf(buffer,"mac_rx %d %s", &port, payload);
            
            if (strlen(payload)) {
                syslog(LOG_DEBUG, "lora: received on port %d: %s", port, payload);
                
                if (lora_rx_callback) {
                     lora_rx_callback(port, payload);
                }
            }

            return LORA_TX_OK;
        } else if (resp == LORA_OTHER) {
            if (outBuffer) {
                strcpy(outBuffer, buffer);
            }

            return LORA_OTHER;
        } else {
            return resp;
        }
    } else {
        return LORA_TIMEOUT;
    }
}

void lora_timer(void *arg) {
    UNUSED_ARG(arg);
  
    if (joined) {
        syslog(LOG_DEBUG, "lora: timer");

        lora_tx(0, 1, "01");
    }
}

// Do a hardware reset
static int lora_hw_reset() {
	// HW reset sequence
    gpio_pin_clr(LORA_RST_PIN);
    delay(50);
    gpio_pin_set(LORA_RST_PIN);

    syslog(LOG_DEBUG, "lora: hw reset");
    
    return lora_response(NULL, 1000 * 5);
}

// Do a reset on Lora module
tdriver_error *lora_reset() {
    char *resp;
	char buff[15];
	
    
    // Reset module by hardware
    if (lora_hw_reset() == LORA_TIMEOUT) {
        syslog(LOG_ERR, "lora: RN2483 not found");

        return setup_error(RES_LORA, "RN2483 not found");        
    }
	
    // Get module version
    resp = lora_sys_get("ver");
    if (resp) {
        if (!strstr(resp,"RN2483")) {
            free(resp);
            syslog(LOG_ERR, "lora: RN2483 not found on %s", uart_name(LORA_UART));
            return setup_error(RES_LORA, "RN2483 not found");
        }
    }
    free(resp);
    
    if (lora_sys("factoryRESET", NULL) != LORA_OK) {
        syslog(LOG_ERR, "lora: RN2483 factory reset fail", uart_name(LORA_UART));
        return setup_error(RES_LORA, "RN2483 factory reset fail");            
    }

    syslog(LOG_INFO, "RN2483 is on %s", uart_name(LORA_UART));
    
	sprintf(buff, "%d", current_band);
	lora_mac("reset", buff);
	
    // Reset the stack, and set default parameters for the selected band
    if (current_band == 868) {
        // Default channel configuration
        // This channels must be implemented in every EU868MHz end-device
        // DR0 to DR5, 1% duty cycle
        lora_mac_set("ch status","0 off");    
        lora_mac_set("ch dcycle","0 99");
        lora_mac_set("ch drrange","0 0 5");
        lora_mac_set("ch status","0 on");

        lora_mac_set("ch status","1 off");    
        lora_mac_set("ch dcycle","1 99");
        lora_mac_set("ch drrange","1 0 5");
        lora_mac_set("ch status","1 on");

        lora_mac_set("ch status","2 off");    
        lora_mac_set("ch dcycle","2 99");
        lora_mac_set("ch drrange","2 0 5");
        lora_mac_set("ch status","2 on");

        // Other channels, in concordance with supported gateways
        // 1% duty cycle
        lora_mac_set("ch freq","3 867100000");
        lora_mac_set("ch dcycle","3 99");
        lora_mac_set("ch drrange","3 0 5");
        lora_mac_set("ch status","3 on");

        lora_mac_set("ch freq","4 867300000");
        lora_mac_set("ch dcycle","4 99");
        lora_mac_set("ch drrange","4 0 5");
        lora_mac_set("ch status","4 on");

        lora_mac_set("ch freq","5 867500000");
        lora_mac_set("ch dcycle","5 99");
        lora_mac_set("ch drrange","5 0 5");
        lora_mac_set("ch status","5 on");

        lora_mac_set("ch freq","6 867700000");
        lora_mac_set("ch dcycle","6 99");
        lora_mac_set("ch drrange","6 0 5");
        lora_mac_set("ch status","6 on");

        lora_mac_set("ch freq","7 867900000");
        lora_mac_set("ch dcycle","7 99");
        lora_mac_set("ch drrange","7 0 5");
        lora_mac_set("ch status","7 on");
    
        lora_mac_set("pwridx","1");
    } else {
        // Default channel configuration
        // This channels must be implemented in every EU433 end-device
        // DR0 to DR5, 1% duty cycle
        lora_mac_set("ch status","0 off");    
        lora_mac_set("ch freq","0 433175000");
        lora_mac_set("ch dcycle","0 99");
        lora_mac_set("ch drrange","0 0 5");
        lora_mac_set("ch status","0 on");

        lora_mac_set("ch status","1 off");    
        lora_mac_set("ch freq","1 433375000");
        lora_mac_set("ch dcycle","1 99");
        lora_mac_set("ch drrange","1 0 5");
        lora_mac_set("ch status","1 on");

        lora_mac_set("ch status","2 off");    
        lora_mac_set("ch freq","2 433575000");
        lora_mac_set("ch dcycle","2 99");
        lora_mac_set("ch drrange","2 0 5");
        lora_mac_set("ch status","2 on");
        
        lora_mac_set("pwridx","0");
    }
    
    lora_mac_set("adr", "off");
    lora_mac_set("ar", "on");
    lora_mac_set("dr", "0");

    // Set deveui with hweui value
    resp = lora_sys_get("hweui");
    lora_mac_set("deveui", resp);
    free(resp);
    
    return NULL;
}

// Setup driver
tdriver_error *lora_setup(int band, int rx_listener) {
    // TO DO: check resources
    syslog(LOG_DEBUG, "lora: setup, band %d, rx listener %d", band, rx_listener);

    if (!setup) {
        gpio_pin_output(LORA_RST_PIN);
        gpio_pin_clr(LORA_RST_PIN);
    
        // Init the UART where RN2483 is attached
		uart_init(LORA_UART, LORA_UART_BR, 0, LORA_UART_BUFF_SIZE);
		uart_init_interrupts(LORA_UART);

        if (!loraTimer && rx_listener) {
            loraTimer = xTimerCreate("loraTimer", LORA_TIMER_FREQ, pdTRUE, (void *) 0, lora_timer);
            if(loraTimer != NULL ){
                xTimerStart(loraTimer, 0);

                syslog(LOG_DEBUG, "lora: timer created");
            }
        }

        uart_setup = 1;
    }
    
    current_band = band;
    
	uart0_swap();
    tdriver_error *error = lora_reset();
	if (error) {
		return error;
	}    
	uart0_default();
    
	setup = 1;

    return NULL;
}

int lora_mac(const char *command, const char *value) {
    int resp;
    char buffer[64];

    mtx_lock(&lora_mtx);

    if (!uart_setup) {
        mtx_unlock(&lora_mtx);
        return LORA_NOT_SETUP;
    }

    if (value) {
        sprintf(buffer, "mac %s %s\r\n", command, value);
    } else {
        sprintf(buffer, "mac %s\r\n", command);
    }

    syslog(LOG_DEBUG, "lora: %s", buffer);
    uart_writes(LORA_UART, buffer);    
    resp = lora_response(NULL, portMAX_DELAY);
    if (resp & (LORA_OK)) {
        mtx_unlock(&lora_mtx);

        return LORA_OK;
    }

    mtx_unlock(&lora_mtx);

    return resp;
}

int lora_sys(const char *command, const char *value) {
    int resp;
    char buffer[64];

    mtx_lock(&lora_mtx);

    if (!uart_setup) {
        mtx_unlock(&lora_mtx);
        return LORA_NOT_SETUP;
    }

    if (value) {
        sprintf(buffer, "sys %s %s\r\n", command, value);
    } else {
        sprintf(buffer, "sys %s\r\n", command);
    }
    
    syslog(LOG_DEBUG, "lora: %s", buffer);
    uart_writes(LORA_UART, buffer);    
    resp = lora_response(NULL, portMAX_DELAY);
    if (resp & (LORA_OK | LORA_OTHER)) {
        mtx_unlock(&lora_mtx);

        return LORA_OK;
    }

    mtx_unlock(&lora_mtx);

    return resp;
}

int lora_mac_set(const char *command, const char *value) {
    int resp;
    char buffer[64];

    mtx_lock(&lora_mtx);

    if (!uart_setup) {
        mtx_unlock(&lora_mtx);
        return LORA_NOT_SETUP;
    }

    sprintf(buffer, "mac set %s %s\r\n", command, value);

    syslog(LOG_DEBUG, "lora: %s", buffer);
    uart_writes(LORA_UART, buffer);    
    resp = lora_response(NULL, portMAX_DELAY);
    if (resp & (LORA_OK)) {
        mtx_unlock(&lora_mtx);

        return LORA_OK;
    }

    mtx_unlock(&lora_mtx);

    return resp;
}

char *lora_mac_get(const char *command) {
    int resp;
    char buffer[64];
    
    sprintf(buffer, "mac get %s\r\n", command);
    
    mtx_lock(&lora_mtx);
    
    syslog(LOG_DEBUG, "lora: %s", buffer);
    uart_writes(LORA_UART, buffer);       
    resp = lora_response(buffer, portMAX_DELAY);
    if (resp & (LORA_OTHER)) {
        char *result = (char *)malloc(strlen(buffer) + 1);
        strcpy(result, buffer);

        mtx_unlock(&lora_mtx);

        return result;
    }
    
    mtx_unlock(&lora_mtx);

    return NULL;
}

char *lora_sys_get(const char *command) {
    int resp;
    char buffer[64];
    
    sprintf(buffer, "sys get %s\r\n", command);
    
    mtx_lock(&lora_mtx);
    
    syslog(LOG_DEBUG, "lora: %s", buffer);
    uart_writes(LORA_UART, buffer); 
    resp = lora_response(buffer, portMAX_DELAY);
    if (resp & (LORA_OTHER)) {
        char *result = (char *)malloc(strlen(buffer) + 1);
        strcpy(result, buffer);

        mtx_unlock(&lora_mtx);

        return result;
    }
    
    mtx_unlock(&lora_mtx);

    return NULL;
}

int lora_join_otaa() {
    int resp;
    int retries = 0;
    
    joined = 0;
    otaa = 1;
    
    mtx_lock(&lora_mtx);

    if (!setup) {
        mtx_unlock(&lora_mtx);
        return LORA_NOT_SETUP;
    }

retry:    
    syslog(LOG_DEBUG, "lora: mac join otaa");
    uart_writes(LORA_UART, "mac join otaa\r\n");    
    resp = lora_response(NULL, portMAX_DELAY);
    if (resp & (LORA_ALL_CHANNELS_BUSY | LORA_DEVICE_DEVICE_IS_NOT_IDLE | LORA_DEVICE_IN_SILENT_STATE)) {
        if (resp & (LORA_ALL_CHANNELS_BUSY)) {
            syslog(LOG_DEBUG, "lora: all channels busy");
        } else {
            syslog(LOG_DEBUG, "lora: not idle or silent");
        }

        if (retries < 3) {
            mtx_unlock(&lora_mtx);
            delay(2000);
            syslog(LOG_DEBUG, "lora: retry");
            retries++;
            mtx_lock(&lora_mtx);
            goto retry;
        }
    } 
    
    if (!(resp & (LORA_OK))) {   
        mtx_unlock(&lora_mtx);

        return resp;
    }

    resp = lora_response(NULL, portMAX_DELAY);
    if (resp & (LORA_JOIN_ACCEPTED)) {
        joined = 1;
        mtx_unlock(&lora_mtx);

        return LORA_OK;
    }
    
    if (resp & (LORA_JOIN_DENIED)) {
        mtx_unlock(&lora_mtx);

        return LORA_JOIN_DENIED;
    }
        
    mtx_unlock(&lora_mtx);
    
    return resp;
}

int lora_tx(int cnf, int port, const char *data) {
    int resp;
    char buffer[1024];
    
    mtx_lock(&lora_mtx);

    if (!setup) {
        mtx_unlock(&lora_mtx);
        return LORA_NOT_SETUP;
    }

    if (cnf) {
        sprintf(buffer,"mac tx cnf %d %s\r\n", port, data);
    } else {
        sprintf(buffer,"mac tx uncnf %d %s\r\n", port, data);        
    }

    syslog(LOG_DEBUG, "lora: %s", buffer);
    uart_writes(LORA_UART, buffer);    
    resp = lora_response(NULL, portMAX_DELAY);
    if (!(resp & (LORA_OK))) {  
        mtx_unlock(&lora_mtx);
        return resp;
    }

    resp = lora_response(NULL, portMAX_DELAY);
    if (resp & (LORA_OK)) {
        mtx_unlock(&lora_mtx);
        return LORA_OK;
    }
        
    mtx_unlock(&lora_mtx);
    
    return resp;
}

void lora_set_rx_callback(lora_rx *callback) {
    lora_rx_callback = callback;
}

#endif
#endif
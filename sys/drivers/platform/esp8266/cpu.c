/*
 * Whitecat, cpu driver
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

#include "FreeRTOS.h"
#include "task.h"

#include "esp/dport_regs.h"
#include <sys/syslog.h>

#include <sys/drivers/gpio.h>
#include <sys/delay.h>

#include <string.h>
/*

#include <time.h>
#include <machine/pic32mz.h>
#include <machine/machConst.h>
#include <drivers/cpu/cpu.h>
#include <drivers/gpio/gpio.h>
#include <drivers/rtc/rtc.h>
#include <drivers/network/network.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
*/

static const char *pin_names[] = {
"?",
"?",
"?",
"?",
"?",
"TOUT",
"?",
"GPIO16",
"GPIO14",
"GPIO12",
"?",
"GPIO13",
"GPIO15",
"GPIO2",
"GPIO0",
"GPIO4",
"?",
"SD_D2",
"SD_D3",
"SD_CMD",
"SD_CLK",
"SD_D0",
"SD_D1",
"GPIO5",
"GPIO3",
"GPIO1",
"?",
"?",
"?",
"?",
"?",
"?",
};

extern void sdk_system_restart_in_nmi(void);
extern uint8_t sdk_rtc_get_reset_reason(void);

/*

typedef enum {
    EC = 0x43, EF = 0x46
} cpu_family;

typedef enum {
    E = 0x43, F = 0x46, G = 0x47, H = 0x48, K = 0x4b, M = 0x4d
} cpu_keyfeat;
*/

/*
static unsigned char cpu_rev = 0;
static unsigned char pins;
static unsigned int flashsz;
static unsigned int ramsz;
static cpu_family family;
static cpu_keyfeat keyfeat;
static unsigned char uart;
static unsigned char spi;
static unsigned char extint;
static unsigned char can;
static unsigned char crypt;
static unsigned char dma;
static unsigned char adc;
static unsigned char i2c;
static unsigned char spi;

static unsigned int *assigned_pins;
*/

/*
// Available adc pins for each port (64 pin parts)
static unsigned int port_adc_pin_mask_64(unsigned int port) {
    switch (port) {
        case 1: return 0b0000000000000000; // PORT A
        case 2: return 0b1111111111111111; // PORT B
        case 3: return 0b0000000000000000; // PORT C
        case 4: return 0b0000000000000000; // PORT D
        case 5: return 0b0000000011110000; // PORT E
        case 7: return 0b0000000000000000; // PORT F
        case 8: return 0b0000001111000000; // PORT G
    }
}

// Get the silicon pin for a pin
static unsigned int pin_number_64(unsigned int pin) {
    switch (pin) {
        case RP('E',5) : return 1; 
        case RP('E',6) : return 2; 
        case RP('E',7) : return 3; 
        case RP('G',6) : return 4; 
        case RP('G',7) : return 5; 
        case RP('G',8) : return 6; 
        case RP('G',9) : return 10; 
        case RP('B',5) : return 11; 
        case RP('B',4) : return 12; 
        case RP('B',3) : return 13; 
        case RP('B',2) : return 14; 
        case RP('B',1) : return 15; 
        case RP('B',0) : return 16; 
        case RP('B',6) : return 17; 
        case RP('B',7) : return 18; 
        case RP('B',8) : return 21; 
        case RP('B',9) : return 22; 
        case RP('B',10): return 23; 
        case RP('B',11): return 24; 
        case RP('B',12): return 27; 
        case RP('B',13): return 28; 
        case RP('B',14): return 29; 
        case RP('B',15): return 30; 
        case RP('C',12): return 31; 
        case RP('C',15): return 32; 
        case RP('F',3) : return 38; 
        case RP('F',4) : return 41; 
        case RP('F',5) : return 42; 
        case RP('D',9) : return 43; 
        case RP('D',10): return 44; 
        case RP('D',11): return 45; 
        case RP('D',0) : return 46; 
        case RP('C',13): return 47; 
        case RP('C',14): return 48; 
        case RP('D',1) : return 49; 
        case RP('D',2) : return 50; 
        case RP('D',3) : return 51; 
        case RP('D',4) : return 52; 
        case RP('D',5) : return 53; 
        case RP('F',0) : return 56; 
        case RP('F',1) : return 57; 
        case RP('E',0) : return 58; 
        case RP('E',1) : return 61; 
        case RP('E',2) : return 62; 
        case RP('E',3) : return 63; 
        case RP('E',4) : return 64; 
    }
    
    return 0;
}
*/

unsigned int cpu_port_number(unsigned int pin) {
	return 0;
}

unsigned int cpu_pin_number(unsigned int pin) {
	return pin;
}

const char *cpu_pin_name(unsigned int pin) {
    return pin_names[pin];
}

unsigned int cpu_port_io_pin_mask(unsigned int port) {
	return 0b11111000000111110;
}

unsigned int cpu_has_gpio(unsigned int port, unsigned int bit) {
	return (cpu_port_io_pin_mask(port) & (1 << bit));
}

unsigned int cpu_has_port(unsigned int port) {
	return (port == 0);
}

/*
unsigned int cpu_port_adc_pin_mask(unsigned int port) {
    switch (pins) {
        case 64: return port_adc_pin_mask_64(port);
    }
}
*/

/*
void cpu_init() {
    cpu_rev = DEVID >> 28;
    
    uart = 6;
    extint = 5;
    can = 0;
    adc = 24;
    spi = 4;
    dma = 8;
    i2c = 4;
    crypt = 0;
    
    switch (DEVID & 0x0fffffff) {
        case 0x05103053: pins = 64;  flashsz = 1024;family = EC;keyfeat = G;break; // PIC32MZ1024ECG064
        case 0x05108053: pins = 64;  flashsz = 1024;family = EC;keyfeat = H;break; // PIC32MZ1024ECH064
        case 0x05130053: pins = 64;  flashsz = 1024;family = EC;keyfeat = M;break; // PIC32MZ1024ECM064
        case 0x05104053: pins = 64;  flashsz = 2048;family = EC;keyfeat = G;break; // PIC32MZ2048ECG064
        case 0x05109053: pins = 64;  flashsz = 2048;family = EC;keyfeat = H;break; // PIC32MZ2048ECH064
        case 0x05131053: pins = 64;  flashsz = 2048;family = EC;keyfeat = M;break; // PIC32MZ2048ECM064
        case 0x0510D053: pins = 100; flashsz = 1024;family = EC;keyfeat = G;break; // PIC32MZ1024ECG100
        case 0x05112053: pins = 100; flashsz = 1024;family = EC;keyfeat = H;break; // PIC32MZ1024ECH100
        case 0x0513A053: pins = 100; flashsz = 1024;family = EC;keyfeat = M;break; // PIC32MZ1024ECM100
        case 0x0510E053: pins = 100; flashsz = 2048;family = EC;keyfeat = G;break; // PIC32MZ2048ECG100
        case 0x05113053: pins = 100; flashsz = 2048;family = EC;keyfeat = H;break; // PIC32MZ2048ECH100
        case 0x0513B053: pins = 100; flashsz = 2048;family = EC;keyfeat = M;break; // PIC32MZ2048ECM100
        case 0x05117053: pins = 124; flashsz = 1024;family = EC;keyfeat = G;break; // PIC32MZ1024ECG124
        case 0x0511C053: pins = 124; flashsz = 1024;family = EC;keyfeat = H;break; // PIC32MZ1024ECH124
        case 0x05144053: pins = 124; flashsz = 1024;family = EC;keyfeat = M;break; // PIC32MZ1024ECM124
        case 0x05118053: pins = 124; flashsz = 2048;family = EC;keyfeat = G;break; // PIC32MZ2048ECG124
        case 0x0511D053: pins = 124; flashsz = 2048;family = EC;keyfeat = H;break; // PIC32MZ2048ECH124
        case 0x05145053: pins = 124; flashsz = 2048;family = EC;keyfeat = M;break; // PIC32MZ2048ECM124
        case 0x05121053: pins = 144; flashsz = 1024;family = EC;keyfeat = G;break; // PIC32MZ1024ECG144
        case 0x05126053: pins = 144; flashsz = 1024;family = EC;keyfeat = H;break; // PIC32MZ1024ECH144
        case 0x0514E053: pins = 144; flashsz = 1024;family = EC;keyfeat = M;break; // PIC32MZ1024ECM144
        case 0x05122053: pins = 144; flashsz = 2048;family = EC;keyfeat = G;break; // PIC32MZ2048ECG144
        case 0x05127053: pins = 144; flashsz = 2048;family = EC;keyfeat = H;break; // PIC32MZ2048ECH144
        case 0x0514F053: pins = 144; flashsz = 2048;family = EC;keyfeat = M;break; // PIC32MZ2048ECM144

        case 0x07209053: pins = 64;  flashsz = 2048;family = EF;keyfeat = H;break; // PIC32MZ2048EFH064
        case 0x07231053: pins = 64;  flashsz = 2048;family = EF;keyfeat = M;break; // PIC32MZ2048EFM064
        }   

    // Key feature, determines presence of CAN / CRYPTO
    switch (keyfeat) {
        case F: can = 2; break;
        case H: can = 2; break;
        case K: can = 2; crypt = 1; break;
        case M: can = 2; crypt = 1; break;
    }
    
    // SPI and I2C ports depends on device pin number
    switch (pins) {
        case 100:
        case 124:
        case 144:
            spi = 6;
            i2c = 5;
    }
    
    // Number of ADC depends on device pin number
    switch (pins) {
        case 100: adc = 40; break;
        case 124: adc = 48; break;
        case 144: adc = 48; break;
    }

    crypt = (keyfeat == M?1:0);
    
    // Allocate array for assigned pins
    assigned_pins = malloc(sizeof(unsigned int) * pins);
}
*/

void cpu_model(char *buffer) {
	if (DPORT.OTP_CHIPID & 0x8000) {
		strcpy(buffer,"ESP8266");
		return;
	}

	*buffer = 0x00;
}

int cpu_speed() {
	if (DPORT.CPU_CLOCK & 0x01) {
		return 160;
	} else {
		return 80;
	}
}

int cpu_revission() {
    return 0;
}

void cpu_show_info() {
	char buffer[18];
    
	cpu_model(buffer);
	if (!*buffer) {
        syslog(LOG_ERR, "cpu unknown CPU");
        syslog(LOG_ERR, "cpu device 0x%x", DPORT.OTP_CHIPID);        		
	} else {
        syslog(LOG_INFO, "cpu %s at %d Mhz", buffer, cpu_speed());        		
	}
}

void cpu_sleep(unsigned int seconds) {
	sdk_system_deep_sleep(seconds * 1000000);
}

void cpu_reset() {
	cpu_sleep(1);
}

struct bootflags
{
    unsigned char raw_rst_cause : 4;
    unsigned char raw_bootdevice : 4;
    unsigned char raw_bootmode : 4;

    unsigned char rst_normal_boot : 1;
    unsigned char rst_reset_pin : 1;
    unsigned char rst_watchdog : 1;

    unsigned char bootdevice_ram : 1;
    unsigned char bootdevice_flash : 1;
};


int cpu_reset_reason() {
	return sdk_rtc_get_reset_reason();
}					
	
/*
unsigned int cpu_pins() {
    return pins;
}

void cpu_assign_pin(unsigned int pin, unsigned int by) {
    
}

void cpu_release_pin(unsigned int pin) {
    
}

unsigned int cpu_pin_assigned(unsigned int pin) {
    
}

void cpu_idle(int seconds) {
    struct tm *info;
    time_t now;
    
    now = time(NULL);
    info = localtime(&now);
    syslog(LOG_INFO,"cpu enter to iddle mode at %s",asctime(info));

    // Stop network interfaces
    #if USE_ETHERNET
    int restart_en =   (netStop("en")   != NET_NOT_YET_STARTED);
    #endif
    
    #if USE_GPRS
    int restart_gprs = (netStop("gprs") != NET_NOT_YET_STARTED);
    #endif

    #if USER_WIFI
    int restart_wf =   (netStop("wf")   != NET_NOT_YET_STARTED);
    #endif
    
    vTaskSuspendAll();

    // Lock sequence
    SYSKEY = 0;			
    SYSKEY = UNLOCK_KEY_0;
    SYSKEY = UNLOCK_KEY_1;

    OSCCONCLR = (1 << 4);   // set Power-Saving mode to iddle
                 
    // Unlock
    SYSKEY = 0;	

    // Discontinue TMR1 on iddle
    T1CONCLR = (1 << 15);
    T1CONSET = (1 << 13);
    T1CONSET = (1 << 15);

    rtc_alarm_at(now + seconds);
    
    asm ("wait"); // Enter in selected power-saving mode

    RCONCLR = (1 << 4);
    RCONCLR = (1 << 3);
    
    xTaskResumeAll();

    // Update clock from RTC
    rtc_update_clock();

    now = time(NULL);
    info = localtime(&now);
    syslog(LOG_INFO,"cpu exit from iddle mode at %s",asctime(info));
    
    // Restart netork interfaces
    #if USE_ETHERNET
    if (restart_en) {
        netStart("en");
    }
    #endif

    #if USE_GPRS
    if (restart_gprs) {
        netStart("gprs");
    }
    #endif

    #if USE_WIFI
    if (restart_wf) {
        netStart("wf");
    }
    #endif
}
*/
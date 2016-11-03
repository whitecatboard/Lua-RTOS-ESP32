/*
 * Lua RTOS, cpu driver
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

#include <time.h>
#include <machine/pic32mz.h>
#include <machine/machConst.h>
#include <sys/drivers/cpu.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/rtc.h>
//#include <sys/drivers/network.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslog.h>

const char *pin_names[] = {
"E5",
"E6",
"E7",
"G6",
"G7",
"G8",
"?",
"?",
"?",
"G9",
"B5",
"B4",
"B3",
"B2",
"B1",
"B0",
"B6",
"B7",
"?",
"?",
"B8",
"B9",
"B10",
"B11",
"?",
"?",
"B12",
"B13",
"B14",
"B15",
"C12",
"C15",
"?",
"?",
"?",
"?",
"?",
"F3",
"?",
"?",
"F4",
"F5",
"D9",
"D10",
"D11",
"D0",
"C13",
"C14",
"D1",
"D2",
"D3",
"D4",
"D5",
"?",
"?",
"F0",
"F1",
"E0",
"?",
"?",
"E1",
"E2",
"E3",
"E4"
};

typedef enum {
    UFAMILY = 0x00, EC = 0x43, EF = 0x46
} cpu_family;

typedef enum {
    UKEY = 0x00, E = 0x43, F = 0x46, G = 0x47, H = 0x48, K = 0x4b, M = 0x4d
} cpu_keyfeat;

static unsigned char cpu_rev = 0;
static unsigned char pins;
static unsigned int flashsz = 0;
static cpu_family family = UFAMILY;
static cpu_keyfeat keyfeat = UKEY;
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

// Available i/o pins for each port (64 pin parts)
static unsigned int port_io_pin_mask_64(unsigned int port) {
    switch (port) {
        case 1: return 0b0000000000000000; // PORT A
        case 2: return 0b1111111111111111; // PORT B
        case 3: return 0b1111000000000000; // PORT C
        case 4: return 0b0000111000111111; // PORT D
        case 5: return 0b0000000011111111; // PORT E
        case 6: return 0b0000000000111011; // PORT F
        case 7: return 0b0000001111000000; // PORT G
    }
	
	return 0;
}

// Available adc pins for each port (64 pin parts)
static unsigned int port_adc_pin_mask_64(unsigned int port) {
    switch (port) {
        case 1: return 0b0000000000000000; // PORT A
        case 2: return 0b1111111111111111; // PORT B
        case 3: return 0b0000000000000000; // PORT C
        case 4: return 0b0000000000000000; // PORT D
        case 5: return 0b0000000011110000; // PORT E
        case 6: return 0b0000000000000000; // PORT F
        case 7: return 0b0000001111000000; // PORT G
    }
	
	return 0;
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

static unsigned int has_port_64(unsigned int port) {
	return ((port > 1) && (port <= 7));
}

unsigned int cpu_pin_number(unsigned int pin) {
    switch (pins) {
        case 64: return pin_number_64(pin);
    }
	
	return 0;
}

unsigned int cpu_has_port(unsigned int port) {
    switch (pins) {
        case 64: return has_port_64(port);
    }
	
	return 0;
}

unsigned int cpu_has_gpio(unsigned int port, unsigned int bit) {
	return (cpu_port_io_pin_mask(port) & (1 << bit));
}

unsigned int cpu_port_number(unsigned int pin) {
	return (pin >> 4);
}

unsigned int cpu_gpio_number(unsigned int pin) {
	return (pin & 0x0f);
}

const char *cpu_pin_name(unsigned int pin) {
    return pin_names[pin];
}

unsigned int cpu_port_io_pin_mask(unsigned int port) {
    switch (pins) {
        case 64: return port_io_pin_mask_64(port);
    }
	
	return 9;
}

unsigned int cpu_port_adc_pin_mask(unsigned int port) {
    switch (pins) {
        case 64: return port_adc_pin_mask_64(port);
    }
	
	return 0;
}

void _cpu_init() {
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
		default:
			can = 0;
			crypt = 0;
			break;
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

 void cpu_model(char *buffer) {
    sprintf(buffer, "PIC32MZ%04dE%c%c%03d", flashsz, 
            family, keyfeat, pins);
}

int cpu_revission() {
    return cpu_rev;
}

void cpu_show_info() {
    char buffer[18];
    
    if (family == 0) {
        syslog(LOG_ERR, "cpu unknown CPU");
        syslog(LOG_ERR, "cpu device 0x%08x", DEVID & 0x0fffffff);        
    } else {
        cpu_model(buffer);

        syslog(LOG_INFO, "cpu %s rev A%d", buffer, cpu_rev);        
    }
}

void cpu_reset() {
    vTaskSuspendAll();

    SYSKEY = 0x00000000; // write invalid key to force lock
    SYSKEY = 0xAA996655; // write key1 to SYSKEY
    SYSKEY = 0x556699AA; // write key2 to SYSKEY

    // OSCCON is now unlocked
    
    /* set SWRST bit to arm reset */
    RSWRSTSET = 1;
    
    /* read RSWRST register to trigger reset */
    unsigned int dummy = RSWRST;

	// This is for avoid a compilation warning
	dummy++;
	
    /* prevent any unwanted code execution until reset occurs*/
    while(1);
}

unsigned int cpu_pins() {
    return pins;
}

void cpu_assign_pin(unsigned int pin, unsigned int by) {
    
}

void cpu_release_pin(unsigned int pin) {
    
}

unsigned int cpu_pin_assigned(unsigned int pin) {
    return 0;
}

void cpu_sleep(int seconds) {
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

#if USE_RTC
    rtc_alarm_at(now + seconds);
#endif
	
    asm ("wait"); // Enter in selected power-saving mode

    RCONCLR = (1 << 4);
    RCONCLR = (1 << 3);
    
    xTaskResumeAll();

    // Update clock from RTC
    //rtc_update_clock();

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

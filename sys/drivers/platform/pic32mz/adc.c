/*
 * Lua RTOS, ADC driver
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
#include "timers.h"
#include "event_groups.h"

#include <sys/drivers/adc.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/dma.h>

#include <sys/syslog.h>

struct adc {
    unsigned int configured;
};

static struct adc adc[NADC] = {{0}};

// Map adc channel to pin number
static int pin_map(int channel) {
    switch (channel) {
        case 0:  return RP('B',0);
        case 1:  return RP('B',1);
        case 2:  return RP('B',2);
        case 3:  return RP('B',3);
        case 4:  return RP('B',4);
        case 5:  return RP('B',10);
        case 6:  return RP('B',11);
        case 7:  return RP('B',12);
        case 8:  return RP('B',13);
        case 9:  return RP('B',14);
        case 10: return RP('B',15);
        case 11: return RP('G',9);
        case 12: return RP('G',8);
        case 13: return RP('G',7);
        case 14: return RP('G',6);
        case 15: return RP('E',7);
        case 16: return RP('E',6);
        case 17: return RP('E',5);
        case 18: return RP('E',4);
        case 45: return RP('B',5);
        case 46: return RP('B',6);
        case 47: return RP('B',7);
        case 48: return RP('B',8);
        case 49: return RP('B',9);
    }

    return -1;
}

// Get adc module related to channel
static int adc_module(int channel) {
    switch (channel) {
        case 0:
        case 45:
            return 0;

        case 1:
        case 46:
            return 1;

        case 2:
        case 47:
            return 2;

        case 3:
        case 48:
            return 3;

        case 4:
        case 49:
            return 4;
    }
    
    return 7;
}

// If channel is belongs to a dedicated channel, this function determines if
// channel is an alternate input
static int adc_alternate(int channel) {
    return ((channel >= 45) && (channel <= 49));
}

// Geneal ADC setup
void adc_setup(unsigned int ref_type) {
    adc[0].configured = 0;

    //Enable module
    PMD1CLR = ADCMD;
    
    // Turbo channel disabled, bit 31, plus bits 30, 29-27, 26-24
    // Fractional data disabled, bit 23, so using interger data format
    // ADC module disabled, bit 15
    // Continue ADC operation in idle mode, bit 13
    // Analog input charge pump disabled, bit 12
    // Capacitive voltage divisor disabled, bit 11
    // Fast synchronous system clock to ADC control clock disabled, bit 10
    // Fast synchronous peripheripal clock to ADC control clock disabled, bit 9
    // Interrupt vector shift bits, 0 position, bits 6-4
    // Scan trigger high level / positive edge sensitive bit, bit 3
    // DMA buffer legth size, bits 2-0
    AD1CON1 = (0b11 << 21) | // ADC7 uses 12 bit resolution
              (1 << 13)    | // Discontinue ADC operation in iddle mode
              (0b110 << 16); // Scan Trigger Source = TMR3
    
    // Capacitor voltage divider, bits 28-26, to 0pf
    // Sample time for the shared ADC, bits 25-16
    // Band gap/vref voltage ready interrupt enable, bit 15, disabled
    // Band gap/vref voltage fault interrupt enable, bit 14, disabled
    // End of scan interrupt enabled bit, bit 13, disabled
    // Early interrupt request override bit, bit 12, disabled
    // External conversion request interface enable bit, bit 11, disabled
    // Shared ADC early interrupt select bits, bits 10-8
    // Shared ADC clock divider bits, bits 6-0
    AD1CON2 = (0x08 << 16); // Sample time for shared 8 TAD

    // Analog-to-digital clock source, bits 31-30
    // Analog-to-digital control clock divider, bit 29-24, = TQ
    // ADC7 enable bit, bit 23, disabled
    // ADC6 enable bit, bit 22, disabled
    // ADC5 enable bit, bit 21, disabled
    // ADC4 enable bit, bit 20, disabled
    // ADC3 enable bit, bit 19, disabled
    // ADC2 enable bit, bit 18, disabled
    // ADC1 enable bit, bit 17, disabled
    // ADC0 enable bit, bit 16, disabled
    // Voltage reference (VREF), bits 15-13
    // Trigger suspend, bit 12, not blocked
    // Update ready interrupt, bit 11, no interrupt is generated
    // Class 2 & 3 analog input sampling enable bit, bit 9
    // Individual ADC input conversion request bit, bit 8, not trigger
    //
    AD1CON3 = ADCSEL_PBCLK    | // Analog-to-digital clock source
              ADC_CONCLKDIV_2 | // ADC frequency divider
              (ref_type << 13);
    
    
    // Sample time and conversion clock default for dedicated channels
    AD10TIME = 0;
    AD11TIME = 0;
    AD12TIME = 0;
    AD13TIME = 0;
    AD14TIME = 0;
    
    // Default analog inputs for dedicated channels
    AD1TRGMODE = 0;
    
    // All channels configured as single-ended unipolar
    AD1IMCON1 = 0;
    AD1IMCON2 = 0;
    AD1IMCON3 = 0;
    
    // Warm-up
    AD1ANCON = (5 << 24);      // Wake-up = 32 * TAD for all modules
            
    // No interrupts
    AD1GIRQEN1 = 0;
    AD1GIRQEN2 = 0;

    // No scanning is used
    AD1CSS1 = 0;
    AD1CSS2 = 0;
    
    // No digital comparators are used
    AD1CMPCON1 = 0;
    AD1CMPCON2 = 0;
    AD1CMPCON3 = 0;
    AD1CMPCON4 = 0;
    AD1CMPCON5 = 0;
    AD1CMPCON6 = 0;
    
    // No oversampling filters are used
    AD1FLTR1 = 0;
    AD1FLTR2 = 0;
    AD1FLTR3 = 0;
    AD1FLTR4 = 0;
    AD1FLTR5 = 0;
    AD1FLTR6 = 0;   
    
    AD1TRGSNS = 0; // All channels edge trigger
    
    // Global software edge trigger
    AD1TRG1 = 0x01010101;
    AD1TRG2 = 0x01010101;
    AD1TRG3 = 0x01010101;
    
    // No early interrupt
    AD1EIEN1 = 0;
    AD1EIEN2 = 0;
    
    // Turn on ADC module
    AD1CON1 |= (1 << 15);

    // Wait ADC for ready
    while ((!AD1CON2) & (1 << 31)); // Wait until reference voltage is ready
    while (AD1CON2 & (1 << 30));  // Wait if there is a fault with ref. voltage
    
    adc[0].configured = 1;
}

// Setup for an ADC channel
int adc_setup_channel(int channel) {
    int module; // Related ADC module to channel
    int alt;
    
    // Exit if ADC module is not configured
    if (!adc[0].configured) {
        return ADC_NOT_CONFIGURED;
    }
    
    // Calculate PIN from channel number
    int pin = pin_map(channel);
    if (pin < 0) {
        return ADC_CHANNEL_DOES_NOT_EXIST;
    }
    
    // Get ADC module related to channel
    module = adc_module(channel);
    
    // Configure pins
    gpio_enable_analog(pin);
    
    if (module != 7) {
        // Sample time and conversion clock
        *(&AD10TIME + module ) = (0b11 << 24)       | // Always 12-bit res.              
                                 (0b0000001 << 16)  | // Clock freq is 1/2
                                 (5 << 0);            // Sample time 5 TAD
        
        // Select analog inputs
        alt = 0;
        if (adc_alternate(channel)) {
            alt = 0b11;
        }
        
        AD1TRGMODE |=  (alt << (16 + module * 2));
    } 
    
    if (!(AD1ANCON & (1 << module))) {
        // Enable clock to analog circuit
        AD1ANCON |= (1 << module);
    
        // Wait for ADC ready
        while(!(AD1ANCON & (1 << (8 + module))));
    }
    
    if (!(AD1CON3 & (1 << (16 + module)))) {
        // Enable ADC module
        AD1CON3 |= (1 << (16 + module));         
    }
    
    syslog(LOG_INFO, "adc%d: at pin %c%d", channel, gpio_portname(pin), gpio_pinno(pin));
    return 1;
}

int adc_read(int channel) {
    // Exit if ADC module is not configured
    if (!adc[0].configured) {
        return ADC_NOT_CONFIGURED;
    }

    // Trigger a conversion
    AD1CON3 |= (1 << 6);
    
    // Wait for the conversion
    if (channel <= 31) {
        while (!(AD1DSTAT1 & (1 << channel)));
    } else {
        while (!(AD1DSTAT2 & (1 << channel)));        
    }

    return *(&AD1DATA0 + channel);
}

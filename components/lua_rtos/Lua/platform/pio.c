/*
 * Lua RTOS, platform functions for lua pio module
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
 * whatsoever resulting from loss of use, ƒintedata or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "luartos.h"

#if 0
#include "Lua/modules/pio.h"

extern const char pin_name[16];

char platform_pio_port_name( unsigned port ) {
    return (char)(65 + port - 1);
}

int platform_pio_has_port( unsigned port) {
    return (cpu_port_io_pin_mask(port) != 0);
}

int platform_pio_has_pin( unsigned port, unsigned pin ) {
    return (cpu_port_io_pin_mask(port) && (1 << pin));
}

pio_type platform_pio_op( unsigned port, pio_type pinmask, int op ) {
    // ADC pin mask for port (if needed)
    unsigned int adcmask;
    
    // Mask pin mask with i/o port map for delete pins thar aren't i/0
    pinmask = pinmask & cpu_port_io_pin_mask(port + 1);
    
    switch (op)  {
        case PLATFORM_IO_PIN_SET:
            LATSET(port) = pinmask;
            return 1;

        case PLATFORM_IO_PIN_CLEAR:
            LATCLR(port) = pinmask;
            return 1;

        case PLATFORM_IO_PIN_GET:
        case PLATFORM_IO_PORT_GET_VALUE:
            return (PORT(port) & pinmask);

        case PLATFORM_IO_PORT_DIR_INPUT:
        case PLATFORM_IO_PIN_DIR_INPUT:
            PULLUCLR(port) = pinmask;
            PULLDPCLR(port) = pinmask;

            adcmask = cpu_port_adc_pin_mask(port + 1);
            if (adcmask) {
                ANSELCLR(port) = adcmask & pinmask;                
            }
            
            TRISSET(port) = pinmask;
            LATCLR(port) = pinmask;
            return 1;

        case PLATFORM_IO_PORT_DIR_OUTPUT:
        case PLATFORM_IO_PIN_DIR_OUTPUT:
            PULLUCLR(port) = pinmask;
            PULLDPCLR(port) = pinmask;

            adcmask = cpu_port_adc_pin_mask(port + 1);
            if (adcmask) {
                ANSELCLR(port) = adcmask & pinmask;                
            }
                        
            LATCLR(port) = pinmask;
            TRISCLR(port) = pinmask;
            return 1;

        case PLATFORM_IO_PIN_PULLUP:
            PULLUSET(port) = pinmask;
            PULLDPCLR(port) = pinmask;
            return 1;
            
        case PLATFORM_IO_PIN_PULLDOWN:
            PULLUCLR(port) = pinmask;
            PULLDPSET(port) = pinmask;
            return 1;
            
        case PLATFORM_IO_PIN_NOPULL:
            PULLUCLR(port) = pinmask;
            PULLDPCLR(port) = pinmask;
            return 1;
            
        case PLATFORM_IO_PORT_SET_VALUE:
            LAT(port) = pinmask;
            return 1;            
    }
}

#endif

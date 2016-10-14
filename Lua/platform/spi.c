/*
 * Whitecat, platform functions for lua SPI module
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
 * whatsoever resulting from loss of use, ƒintedata or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "whitecat.h"

#if LUA_USE_SPI

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "Lua/modules/spi.h"
#include "drivers/spi/spi.h"

#include <strings.h>

int platform_spi_exists( unsigned id ) {
    return ((id >= 0) && (id <= NSPI));
}

int platform_spi_pins( lua_State* L ) {
    int i;
    unsigned char sdi, sdo, sck;
    
    for(i=1;i<=NSPI;i++) {
        spi_pins(i, &sdi, &sdo, &sck);

        printf(
            "spi%d: sdi=%c%d\t(pin %2d)\tsdo=%c%d\t(pin %2d)\tsck=%c%d\t(pin %2d)\n", i,
            gpio_portname(sdi), gpio_pinno(sdi),cpu_pin_number(sdi),
            gpio_portname(sdo), gpio_pinno(sdo),cpu_pin_number(sdo),
            gpio_portname(sck), gpio_pinno(sck),cpu_pin_number(sck) 
        );
    }
    
    return 0;
}

u32 platform_spi_setup( spi_userdata *spi, int mode, u32 clock, unsigned cpol, unsigned cpha, unsigned databits ) {
    if (spi_init(spi->spi) != 0) {
        return 0;
    }
    
    spi_set_speed(spi->spi, spi->speed);
    spi_set_cspin(spi->spi, spi->cs);
    spi_deselect(spi->spi);
    
    if (mode == 1) {
        spi->mode = PIC32_SPICON_MSTEN | PIC32_SPICON_ON | PIC32_SPICON_CKE | PIC32_SPICON_SMP;
        
        switch (databits) {
            case 8:
                break;
            case 16:
                spi->mode |= PIC32_SPICON_MODE16;
                break;

            case 32:
                spi->mode |= PIC32_SPICON_MODE16;
                break;
        }
        spi_set(spi->spi, spi->mode);
    } else {
        return 0;
    }

    return 0;
}

void platform_spi_select( spi_userdata *spi, int is_select ) {
    spi_set_speed(spi->spi, spi->speed);
    spi_clr_and_set(spi->spi, spi->mode);
    
    if (is_select) {
        spi_select(spi->spi);
    } else  {
        spi_deselect(spi->spi);
    }
}

spi_data_type platform_spi_send_recv( spi_userdata *spi, spi_data_type data ) {
    return spi_transfer(spi->spi, data);
}

#endif
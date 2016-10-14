
/*
 * Whitecat, platform functions for lua UART module
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

#if LUA_USE_UART

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <drivers/uart/uart.h>

int platform_uart_exists( unsigned id ) {
    return ((id > 0) && (id <= NUART));
}

int platform_uart_pins() {
    int i;
    unsigned char rx, tx;
    
    for(i=1;i<=NUART;i++) {
        uart_pins(i, &rx, &tx);

        if ((rx != 0) && (tx != 0)) {
            printf(
                "uart%d: rx=%c%d\t(pin %2d)\ttx=%c%d\t(pin %2d)\n", i,
                gpio_portname(rx), gpio_pinno(rx),cpu_pin_number(rx),
                gpio_portname(tx), gpio_pinno(tx),cpu_pin_number(tx)
            );            
        }
    }
    
    return 0;
}

int platform_uart_setup(lua_State* L, int id, int bauds, int databits, int parity, int stop_bits, int buffer) {
    int mode = 0;
    
    if ((databits == 8) && (parity == 0)) {
        mode = PIC32_UMODE_PDSEL_8NPAR;
    } else if ((databits == 8) && (parity == 1)) {
        mode = PIC32_UMODE_PDSEL_8EVEN;
    } else if ((databits == 8) && (parity == 2)) {
        mode = PIC32_UMODE_PDSEL_8ODD;
    } else if ((databits == 9) && (parity == 0)) {
        mode = PIC32_UMODE_PDSEL_9NPAR;
    } else {
        return luaL_error(L, "invalid data bits / parity combination");               
    }    

    uart_init(id, bauds, mode, buffer);
    uart_init_interrupts(id);
    
    return uart_get_br(id);
} 

void platform_uart_write_byte(int id, int c) {
    if (id == CONSOLE_UART) {
        fwrite(&c, 1, 1, stdout);
    } else {
        uart_write(id, c);
    }
}

void platform_uart_write_string(int id, const char *s) {
    if (id == CONSOLE_UART) {
        fwrite(s, strlen(s) + 1, 1, stdout);
    } else {
        uart_writes(id, (char *)s);
    }
}

int platform_uart_read_string(int id, char *str, int crlf, int timeout) {    
    if (timeout == 0xffffffff) {
        timeout = portMAX_DELAY;
    }
    
    return uart_reads(id, str, crlf, timeout);
}

int platform_uart_read_char(int id, char *c, int timeout) {    
    if (timeout == 0xffffffff) {
        timeout = portMAX_DELAY;
    }
    
    return uart_read(id, c, timeout);
}

int platform_uart_issetup(int id) {
    return uart_inited(id);
}

void platform_consume(int id) {
    uart_consume(id);
}

#endif
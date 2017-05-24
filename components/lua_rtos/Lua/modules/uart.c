/*
 * Lua RTOS, UART wrapper
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
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_UART

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>

#include "lua.h"
#include "lauxlib.h"
#include "uart.h"
#include "error.h"

#include <drivers/gpio.h>
#include <drivers/cpu.h>
#include <drivers/uart.h>

static int uart_exists(int id) {
    return ((id >= CPU_FIRST_UART) && (id <= CPU_LAST_UART));
}

static int luart_setup( lua_State* L ) {
	driver_error_t *error;
	int flags = UART_FLAG_WRITE | UART_FLAG_READ;


	int id = luaL_checkinteger(L, 1);
    int bauds = luaL_checkinteger(L, 2);
    int databits = luaL_checkinteger(L, 3);
    int parity = luaL_checkinteger(L, 4);
    int stop_bits = luaL_checkinteger(L, 5);
    int buffer = luaL_optinteger(L, 6, 1024);

	if (lua_gettop(L) == 7) {
		flags = luaL_checkinteger(L, 7);
	}

    // Setup
    error = uart_init(id, bauds, databits, parity, stop_bits, flags, buffer);
    if (error) {
        return luaL_driver_error(L, error);
    }

    error = uart_setup_interrupts(id);
    if (error) {
        return luaL_driver_error(L, error);
    }

    int real_bauds = uart_get_br(id);
    
    if (real_bauds != 0) {
        lua_pushinteger(L, real_bauds);
        return 1;
    } else {
        return 0;
    }
}

static int luart_write( lua_State* L ) {
    int arguments = lua_gettop(L); 
    int id = luaL_checkinteger(L, 1);
    lua_Integer c;
    const char *s;
    int i;
    
    // Some integrity checks
    if (!uart_exists(id)) {
        return luaL_error(L, "UART%d does not exist", id);
    }

    if (!uart_is_setup(id)) {
        return luaL_error(L, "UART%d is not setup", id);
    }
    
    // Write ...
    for(i=2;i <= arguments; i++) {
        if (lua_type( L, i) == LUA_TNUMBER) {
            c = lua_tointegerx(L, i, NULL);
            if ((c < 0) || (c > 255)) {
                return luaL_error(L, "invalid argument %d (not a byte)", i);                     
            }
            
            if (id == CONSOLE_UART) {
                fwrite(&c, 1, 1, stdout);
            } else {
                uart_write(id, c);
            }
        } else if (lua_type( L, i) == LUA_TSTRING) {
            s = lua_tolstring(L, i, NULL);
            
            if (id == CONSOLE_UART) {
                fwrite(s, strlen(s) + 1, 1, stdout);
            } else {
                uart_writes(id, (char *)s);
            }
        } else {
            return luaL_error(L, "invalid argument %d", i);  
        }
    }
    
    return 0;
} 

static int luart_read( lua_State* L ) {
    int id = luaL_checkinteger(L, 1);
    const char  *format = luaL_checkstring(L, 2);
    int timeout, res, c;
    
    // Some integrity checks
    if (!uart_exists(id)) {
        return luaL_error(L, "UART%d does not exist", id);
    }
    
    if (!uart_is_setup(id)) {
        return luaL_error(L, "UART%d is not setup", id);
    }
    
    timeout = luaL_optinteger(L, 3, 0xffffffff);
    if (timeout == 0xffffffff) {
        timeout = portMAX_DELAY;
    }

    // Read ...
    if (strcmp("*l", format) == 0) {
        char *str = (char *)malloc(sizeof(char) * LUAL_BUFFERSIZE);

        res = uart_reads(id, str, 0, timeout);
        if (res) {
            lua_pushlstring(L, str, strlen(str));
            free(str);
        } else {
            lua_pushnil(L);
        }
        
        return 1;
    } else if (strcmp("*el", format) == 0) {
        char *str = (char *)malloc(sizeof(char) * LUAL_BUFFERSIZE);
        
        res = uart_reads(id, str, 1, timeout);
        if (res) {
            lua_pushlstring(L, str, strlen(str));
            free(str);
        } else {
            lua_pushnil(L);            
        }
        
        return 1;
    } else if (strcmp("*c", format) == 0) {
        res = uart_read(id, (char *)&c, timeout);
        if (res) {
            lua_pushinteger(L, c & 0x000000ff);
        } else {
            lua_pushnil(L);
        }
        
        return 1;
    } else {
        return luaL_error(L, "invalid format");        
    }
    
    
    return 0;
}

static int luart_consume( lua_State* L ) {
	driver_error_t *error;
	int id = luaL_checkinteger(L, 1);
    
    error = uart_consume(id);
    if (error) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

static int luart_lock( lua_State* L ) {
	driver_error_t *error;
	int id = luaL_checkinteger(L, 1);

    error = uart_lock(id);
    if (error) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

static int luart_unlock( lua_State* L ) {
	driver_error_t *error;
	int id = luaL_checkinteger(L, 1);

    error = uart_unlock(id);
    if (error) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

#include "modules.h"

static const LUA_REG_TYPE uart_map[] = {
    { LSTRKEY( "setup"    ),	 LFUNCVAL( luart_setup ) },
    { LSTRKEY( "write"    ),	 LFUNCVAL( luart_write ) },
    { LSTRKEY( "read"     ),	 LFUNCVAL( luart_read ) },
    { LSTRKEY( "consume"  ),	 LFUNCVAL( luart_consume ) },
    { LSTRKEY( "lock"     ),	 LFUNCVAL( luart_lock ) },
    { LSTRKEY( "unlock"   ),	 LFUNCVAL( luart_unlock ) },
	{ LSTRKEY( "CONSOLE"  ),	 LINTVAL ( CONSOLE_UART ) },
	{ LSTRKEY( "PARNONE"  ),	 LINTVAL ( 0 ) },
	{ LSTRKEY( "PAREVEN"  ),	 LINTVAL ( 1 ) },
	{ LSTRKEY( "PARODD"   ),	 LINTVAL ( 2 ) },
	{ LSTRKEY( "STOPHALF" ),	 LINTVAL ( 0 ) },
	{ LSTRKEY( "STOP1"    ),	 LINTVAL ( 1 ) },
	{ LSTRKEY( "STOP2"    ),	 LINTVAL ( 2 ) },
    UART_UART0
    UART_UART1
    UART_UART2
	{ LNILKEY, LNILVAL }
};

int luaopen_uart(lua_State* L) {
    return 0;
}

MODULE_REGISTER_MAPPED(UART, uart, uart_map, luaopen_uart);

#endif

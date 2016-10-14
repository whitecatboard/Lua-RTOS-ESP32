/*
 * Whitecat, uart wrapper for whitecat
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

#if LUA_USE_UART

#include <stdlib.h>
#include <assert.h>
#include <signal.h>

#include "lua.h"
#include "lauxlib.h"

#include <sys/statust.h>
#include <drivers/uart/uart.h>

static int luart_pins( lua_State* L ) {
    return platform_uart_pins();
}

static int luart_setup( lua_State* L ) {
    int id = luaL_checkinteger(L, 1);
    int bauds = luaL_checkinteger(L, 2);
    int databits = luaL_checkinteger(L, 3);
    int parity = luaL_checkinteger(L, 4);
    int stop_bits = luaL_checkinteger(L, 5);
    int buffer = luaL_optinteger(L, 6, 1024);

    // Some integrity checks
    if (!platform_uart_exists(id)) {
        return luaL_error(L, "UART %d does not exist", id);
    }
    
    if ((databits != 8) && (databits != 9)) {
        return luaL_error(L, "invalid data bits");        
    }
            
    if ((parity < 0) || (parity > 2)) {
        return luaL_error(L, "invalid parity");                
    }
    
    if ((stop_bits != 1) && (stop_bits != 2)) {
        return luaL_error(L, "invalid stop bits");                        
    }
            
    // Setup
    int real_bauds = platform_uart_setup(L, id, bauds, databits, parity, stop_bits, buffer);
    
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
    if (!platform_uart_exists(id)) {
        return luaL_error(L, "UART %d does not exist", id);
    }

    if (!platform_uart_issetup(id)) {
        return luaL_error(L, "UART %d is not setup", id);        
    }
    
    // Write ...
    for(i=2;i <= arguments; i++) {
        if (lua_type( L, i) == LUA_TNUMBER) {
            c = lua_tointegerx(L, i, NULL);
            if ((c < 0) || (c > 255)) {
                return luaL_error(L, "invalid argument %d (not a byte)", i);                     
            }
            
            platform_uart_write_byte(id, c);            
        } else if (lua_type( L, i) == LUA_TSTRING) {
            s = lua_tolstring(L, i, NULL);
            
            platform_uart_write_string(id, s);
        } else {
            return luaL_error(L, "invalid argument %d", i);  
        }
    }
    
    return 0;
} 

static int luart_read( lua_State* L ) {
    int id = luaL_checkinteger(L, 1);
    const char  *format = luaL_checkstring(L, 2);
    int timeout, crlf, res, c;
    
    // Some integrity checks
    if (!platform_uart_exists(id)) {
        return luaL_error(L, "UART %d does not exist", id);
    }
    
    if (!platform_uart_issetup(id)) {
        return luaL_error(L, "UART %d is not setup", id);        
    }
    
    // Read ...
    if (strcmp("*l", format) == 0) {
        luaL_checktype(L, 3, LUA_TBOOLEAN);
        crlf = lua_toboolean( L, 3 );

        timeout = luaL_optinteger(L, 4, 0xffffffff);
        
        char *str = (char *)malloc(sizeof(char) * LUAL_BUFFERSIZE);
        
        res = platform_uart_read_string(id, str, crlf, timeout);
        if (res) {
            lua_pushlstring(L, str, strlen(str));
            free(str);
        } else {
            lua_pushnil(L);            
        }
        
        return 1;
    } else if (strcmp("*c", format) == 0) {
        timeout = luaL_optinteger(L, 3, 0xffffffff);
        res = platform_uart_read_char(id, &c, timeout);
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
    int id = luaL_checkinteger(L, 1);
    
    // Some integrity checks
    if (!platform_uart_exists(id)) {
        return luaL_error(L, "UART %d does not exist", id);
    }
    
    if (!platform_uart_issetup(id)) {
        return luaL_error(L, "UART %d is not setup", id);        
    }
    
    platform_consume(id);
    
    return 0;
}

static const luaL_Reg uart[] = {
    {"pins", luart_pins},
    {"setup", luart_setup},
    {"write", luart_write},
    {"read", luart_read},
    {"consume", luart_consume},
    {NULL, NULL}
};

int luaopen_uart(lua_State* L)
{
    luaL_newlib(L, uart);

    lua_pushinteger(L, CONSOLE_UART);
    lua_setfield(L, -2, "CONSOLE");

    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "PARNONE");

    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "PAREVEN");

    lua_pushinteger(L, 2);
    lua_setfield(L, -2, "PARODD");

    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "STOP1");

    lua_pushinteger(L, 2);
    lua_setfield(L, -2, "STOP2");

    int i;
    char buff[6];

    for(i=1;i<=NUART;i++) {
        sprintf(buff,"UART%d",i);
        lua_pushinteger(L, i);
        lua_setfield(L, -2, buff);
    }
    
    return 1;
}

#endif
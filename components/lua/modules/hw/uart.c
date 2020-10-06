/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS, Lua UART module
 *
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

extern struct uart uart[NUART];

static int uart_exists(int id) {
    return ((id >= CPU_FIRST_UART) && (id <= CPU_LAST_UART));
}

static int luart_pins(lua_State* L) {
	uint8_t table = 0;
	uint16_t count = 0, i = 0;
	int unit = CPU_FIRST_UART;

	// Check if user wants result as a table, or wants result
	// on the console
	if (lua_gettop(L) == 1) {
		luaL_checktype(L, 1, LUA_TBOOLEAN);
		if (lua_toboolean(L, 1)) {
			table = 1;
		}
	}

	if (table){
		lua_createtable(L, count, 0);
	}

	for(unit = CPU_FIRST_UART; unit <= CPU_LAST_UART;unit++) {
		if (!table) {
			printf("UART%d: ", unit);

			printf("rx=");
			if (uart[unit].rx >= 0) {
				printf("%s%d ", gpio_portname(uart[unit].rx), gpio_name(uart[unit].rx));
			} else {
				printf("unused");
			}

			printf("tx=");
			if (uart[unit].tx >= 0) {
				printf("%s%d ", gpio_portname(uart[unit].tx), gpio_name(uart[unit].tx));
			} else {
				printf("unused");
			}

			printf("\r\n");
		} else {
			lua_pushinteger(L, i);

			lua_createtable(L, 0, 3);

			lua_pushinteger(L, unit);
	        lua_setfield (L, -2, "id");

	        lua_pushinteger(L, uart[unit].rx);
			lua_setfield (L, -2, "rx");

			lua_pushinteger(L, uart[unit].tx);
	        lua_setfield (L, -2, "tx");

			 lua_settable(L,-3);
		}

		i++;
	}

	return table;
}

static int luart_setpins(lua_State* L) {
	driver_error_t *error;

	int id = luaL_checkinteger(L, 1);
	int rx = luaL_checkinteger(L, 2);
	int tx = luaL_checkinteger(L, 3);

	if ((error = uart_pin_map(id, rx, tx))) {
	    return luaL_driver_error(L, error);
	}

	return 0;
}

static int luart_setup( lua_State* L ) {
	driver_error_t *error;
	int flags = UART_FLAG_WRITE | UART_FLAG_READ;

	luaL_deprecated(L, "uart.setup", "uart.attach");

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

static int luart_attach( lua_State* L ) {
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

            uart_write(id, c);
        } else if (lua_type( L, i) == LUA_TSTRING) {
            s = lua_tolstring(L, i, NULL);
            uart_writes(id, (char *)s);
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
        if (NULL == str) {
            return luaL_error(L, "could not allocate memory to read *l");
        }

        res = uart_reads(id, str, 0, timeout);
        if (res) {
            lua_pushlstring(L, str, strlen(str));
        } else {
            lua_pushnil(L);
        }

        free(str);
        return 1;
    } else if (strcmp("*el", format) == 0) {
        char *str = (char *)malloc(sizeof(char) * LUAL_BUFFERSIZE);
        if (NULL == str) {
            return luaL_error(L, "could not allocate memory to read *el");
        }
        
        res = uart_reads(id, str, 1, timeout);
        if (res) {
            lua_pushlstring(L, str, strlen(str));
        } else {
            lua_pushnil(L);
        }

        free(str);
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
    { LSTRKEY( "attach"   ),	 LFUNCVAL( luart_attach ) },
    { LSTRKEY( "pins"     ),	 LFUNCVAL( luart_pins ) },
	{ LSTRKEY( "setpins"  ),	 LFUNCVAL( luart_setpins  ) },
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

MODULE_REGISTER_ROM(UART, uart, uart_map, luaopen_uart, 1);

#endif

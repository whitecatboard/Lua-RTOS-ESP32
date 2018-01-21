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
 * Lua RTOS, Lua io library additions
 *
 */

#include "luartos.h"

#include "lua.h"
#include "lauxlib.h"

#include <sys/dirent.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/syslog.h>
#include <drivers/uart.h>

#define l_getc(f)		getc(f)
#define l_lockfile(f)   ((void)0)
#define l_unlockfile(f)	((void)0)

int ioctl(int fd, unsigned long request, ...);

static int f_attributes(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    char type[10];

    struct stat s;

    if (stat(path, &s) < 0) {
    	lua_pushnil(L);

    	return 1;
    }

	if (s.st_mode == S_IFDIR) {
		strcpy(type, "directory");
	} else if (s.st_mode == S_IFREG) {
		strcpy(type, "file");
	}

	lua_createtable(L, 0, 2);

	lua_pushstring(L, "type");
	lua_pushstring(L, type);
	lua_settable(L, -3);

	lua_pushstring(L, "size");
	lua_pushinteger(L, s.st_size);
	lua_settable(L, -3);

	return 1;
}

static int read_line (lua_State *L, FILE *f, int chop) {
  luaL_Buffer b;
  char c = '\0';

  luaL_buffinit(L, &b);
  
  char nl = '\n';
  
  if (f == stdin) {
      nl = '\r';
  }
  
  while ((c != (char)EOF) && (c != nl)) {  /* repeat until end of line */
    char *buff = luaL_prepbuffer(&b);  /* pre-allocate buffer */
    int i = 0;
    l_lockfile(f);  /* no memory errors can happen inside the lock */
    while ((i < LUAL_BUFFERSIZE) && ((c = l_getc(f)) != (char)EOF) && (c != nl)) {
      buff[i++] = c;
    
      if (f == stdin) {
          putc(c, stdout);
      }
    }
    
    l_unlockfile(f);
    luaL_addsize(&b, i);
  }
  if (!chop && c == nl)  /* want a newline and have one? */
    luaL_addchar(&b, c);  /* add ending newline to result */
  luaL_pushresult(&b);  /* close buffer */
  /* return ok if read something (either a newline or something else) */
  return (c == nl || lua_rawlen(L, -1) > 0);
}

static int f_receive (lua_State *L) {
    const char *filename = luaL_optstring(L, 1, "");
    unsigned int i;
    int done;
    int bytes = 0;

    unsigned char chunk[255];
    unsigned char *cchunk;
    unsigned char chunk_size;

    int buff_size = 10240;

    if (strlen(filename) == 0) return 0;

    FILE *f= fopen(filename, "w");
    if (f) {
    	// Try to allocate a great buffer for output stream
    	while (setvbuf(f , NULL, _IOFBF, buff_size) != 0) {
    		buff_size = buff_size - 1024;
    	}

        uart_ll_lock(CONSOLE_UART);
        uart_ll_set_raw(1);
        
        // Clear received buffer
        uart_consume(CONSOLE_UART);

        // Send 'C' for start
        uart_write(CONSOLE_UART, 'C');
        uart_write(CONSOLE_UART, '\n');

        done = 0;

        for(;;) {
            // Wait for chunk size
            if (!uart_read(CONSOLE_UART, (char *)&chunk_size, 2000)) {
                break;
            }

            // More chunks?
            if (chunk_size == 0) {
                done = 1;
                break;
            }

            // Read chunk
            cchunk = chunk;
            for(i=0; i < chunk_size; i++) {
                if (!uart_read(CONSOLE_UART, (char *)cchunk++, 2000)) {
                    break;
                }
                bytes++;
            }

            // Wrhite chunk to disk
            fwrite(chunk,chunk_size,1,f);

            // Send 'C' for start
            uart_write(CONSOLE_UART, 'C');
            uart_write(CONSOLE_UART, '\n');
        }

        fclose(f);
        
        if (!done) {
            uart_ll_set_raw(0);
            uart_ll_unlock(CONSOLE_UART);
            
            return luaL_error(L, "timeout");
        }

        uart_ll_set_raw(0);
        uart_ll_unlock(CONSOLE_UART);
    } else {
        return luaL_error(L, strerror(errno));
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int f_send (lua_State *L) {
    const char *filename = luaL_optstring(L, 1, "");
    int i;
    int done;
    int error;
    char c;

    unsigned char chunk[255];
    unsigned char *cchunk;
    unsigned char chunk_size;
  
    if (strlen(filename) == 0) return 0;

    FILE *f= fopen(filename, "r");
    if (f) {
    	uart_ll_lock(CONSOLE_UART);
    	uart_ll_set_raw(1);

        done = 1;
        error = 0;
        while (!feof(f)) {
            // Read next chunk
            chunk_size = 0;
            cchunk = chunk;
            for(i=0; i < 255; i++) {
                if (fread(cchunk++,1,1,f) == 1) {
                    chunk_size++;
                }
            }
            
            // Wait for C\n
            if (!uart_read(CONSOLE_UART, &c, 2000)) {done = 0; break;}
            if (c != 'C') {done = 0; error = 1; break;}
            
            if (!uart_read(CONSOLE_UART, &c, 2000)) {done = 0; break;}
            if (c != '\n') {done = 0; error = 1; break;}
            
            // Send chunk size
            uart_write(CONSOLE_UART, chunk_size);

            // Send chunk
            cchunk = chunk;
            while(chunk_size--) {
                uart_write(CONSOLE_UART, *cchunk++);
            }  
        }
                
        fclose(f);
        
        if (done){
            // Send last chunk with size 0

            // Wait for C\n
            if (!uart_read(CONSOLE_UART, &c, 2000)) {
            	uart_ll_set_raw(0);
            	uart_ll_unlock(CONSOLE_UART);
            	return luaL_error(L, "timeout");
            }

            if (c != 'C') {
            	uart_ll_set_raw(0);
            	uart_ll_unlock(CONSOLE_UART);
            	return luaL_error(L, "unnexpected input");
            }

            if (!uart_read(CONSOLE_UART, &c, 2000)) {
            	uart_ll_set_raw(0);
            	uart_ll_unlock(CONSOLE_UART);
            	return luaL_error(L, "timeout");
            }
            
            if (c != '\n') {
            	uart_ll_set_raw(0);
            	uart_ll_unlock(CONSOLE_UART);
            	return luaL_error(L, "unnexpected input");
            }
            
            // Send chunk size
            chunk_size = '\0';
            uart_write(CONSOLE_UART, chunk_size);
        } else {
        	uart_ll_set_raw(0);
            uart_ll_unlock(CONSOLE_UART);

            if (error) {
            	return luaL_error(L, "unnexpected input");
            } else {
            	return luaL_error(L, "timeout");
            }
        }

        uart_ll_set_raw(0);
        uart_ll_unlock(CONSOLE_UART);

    } else {
        return luaL_error(L, strerror(errno));
    }

    return 0;
}

#undef l_getc
#undef l_lockfile
#undef l_unlockfile

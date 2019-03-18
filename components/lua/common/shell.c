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
 * Lua RTOS shell
 *
 */

#include "shell.h"

#include "lua.h"
#include "lauxlib.h"
#include "lapi.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    const char *command;
    const char *module;
    const char *function;
    const uint8_t mandatory;
    const uint8_t optional;
    const char *usage;
} command_t;

typedef struct {
    char *tb;
    char *te;
} token_t;

static const command_t command[] = {
    {"luac", NULL, "compile", 1, 0, "luac source destination"},
    {"cat", "os", "cat", 1, 0, "cat filename"},
    {"cd", "os", "cd", 0, 1, "cd path"},
    {"cp", "os", "cp", 2, 0, "cp from to"},
    {"dmesg", "os", "dmesg", 0, 0, NULL},
    {"do", "_G", "dofile", 1, 0, "do [lua script]"},
    {"clear", "os", "clear", 0, 0, NULL},
    {"edit", "os", "edit", 1, 0, "edit [filename]"},
    {"exit", "os", "exit", 0, 0, NULL},
    {"reboot", "os", "exit", 0, 0, NULL},
    {"ls", "os", "ls", 0, 1, "ls [pattern]"},
    {"dir", "os", "ls", 0, 1, "dir [pattern]"},
    {"luac", NULL, "compile", 1, 0, "luac source destination"},
    {"luad", NULL, "decompile", 1, 0, "luad source"},
    {"mkfs", "os", "format", 1, 0, "format path"},
    {"mkdir", "os", "mkdir", 1, 0, "mkdir path"},
    {"mount", "fs", "mount", 2, 0, "target filesystem"},
    {"more", "os", "more", 1, 0, "more filename"},
    {"mv", "os", "rename", 2, 0, "mv old new"},
    {"netstat", "net", "stat", 0, 0, NULL},
    {"passwd", "os", "passwd", 0, 0, NULL},
    {"ping", "net", "ping", 1, 0, "ping destination"},
    {"pwd", "os", "pwd", 0, 0, NULL},
    {"remove", "os", "remove", 1, 0, "remove filename"},
    {"rename", "os", "rename", 2, 0, "rename old new"},
    {"rm", "os", "remove", 1, 0, "rm filename"},
    {"umount", "fs", "umount", 1, 0, "target"},
    {"unlink", "os", "remove", 1, 0, "unlink filename"},
    {"uptime", "os", "uptime", 0, 0, NULL},
    {NULL, NULL, NULL, 0, 0, NULL},
};

static char *lua_token_skip_spaces(char *buffer) {
    char *cbuffer = buffer;

    while ((*cbuffer) && ((*cbuffer == ' ') || (*cbuffer == '\r') || (*cbuffer == '\n'))) {
        cbuffer++;
    }

    return cbuffer;
}

static char *lua_token_skip_valid(char *buffer) {
    char *cbuffer = buffer;

    while ((*cbuffer) && ((*cbuffer != ' ') && (*cbuffer != '\r') && (*cbuffer != '\n'))) {
        cbuffer++;
    }

    cbuffer--;

    return cbuffer;
}

static int get_command(char *tb, char *te) {
    char *ct,*cc;
    int i;
    int tl;

    i = 0;
    while (command[i].command ) {
        ct = (char *)tb;
        cc = (char *)(command[i].command);
        tl = 0;
        while ((*ct) && (*cc) && (ct <= te) && (*ct == *cc)) {
            ct++;
            cc++;
            tl++;
        }

        if ((ct > te) && (strlen(command[i].command) == tl)) {
            return i;
        }

        i++;
    }

    return -1;
}

static void get_args_from_shell_command(char *buffer, char *filename, char *args) {
    char* pos = args;

    //cut off trailing any spaces
    for(char* trimbuf = buffer + strlen(buffer) - 1; trimbuf > buffer && *trimbuf==' '; trimbuf--)
        *trimbuf=0;

    char* param = buffer + strlen(filename);
    //skip leading spaces
    while(*param==' ') param++;

    bool found = false;
    while((pos - args) < 250 && *param != 0) {
        if (!found) {
            *pos = '"';
            pos++;
            found = true;
        }

        if (*param==' ') {
            *pos = '"';

            if (*(param+1) != 0) {
                pos++;
                *pos = ',';
                pos++;
                *pos = '"';
            }

            while(*param==' ') param++;
        }
        else {
            *pos = *param;
            param++;
        }
        pos++;
    }

    if (found) {
        *pos = '"';
        pos++;
    }

    *pos = 0;
    return;
}

void lua_shell(lua_State* L, char *buffer) {
    char *cbuffer = buffer;
    int itoken = 0;
    int cindex = -1;
    char *arg;
    token_t *tokens;

    arg = calloc(1, LUA_MAXINPUT);
    if (!arg) {
        goto exit;
    }

    tokens = calloc(10, sizeof(token_t));
    if (!tokens) {
        goto exit;
    }

    // Search tokens in buffer ...
    while (*cbuffer && (itoken < 10)) {
        cbuffer = lua_token_skip_spaces(cbuffer);
        tokens[itoken].tb = cbuffer;
        tokens[itoken].te = cbuffer;

        if (*cbuffer) {
            cbuffer = lua_token_skip_valid(cbuffer);
            if (*cbuffer) {
                tokens[itoken].te = cbuffer;

                if (itoken == 0) {
                    // If it's first token, search token into the shell command list.
                    // If a command is not find exit.
                    if ((cindex = get_command(tokens[itoken].tb, tokens[itoken].te)) < 0) {
                        itoken++;
                        break;
                    }
                }

                cbuffer++;
                itoken++;
            }
        }
    }

    if (itoken >= 10) {
        goto exit;
    }

    // Process command
    if (cindex >= 0) {
        // Check for mandatory arguments
        if ((itoken - 1) < command[cindex].mandatory) {
            if (command[cindex].usage) {
                printf("usage: %s\r\n", command[cindex].usage);
                *buffer = 0x00;

                goto exit;
            }
        }

        // Call to corresponding module / function
        if (command[cindex].module) {
            lua_getglobal(L, command[cindex].module);
            lua_getfield(L, -1, command[cindex].function);
        } else {
            lua_getglobal(L, command[cindex].function);
        }

        // Prepare arguments
        int i, args = 0;

        for(i = 1; i < itoken; i++) {
            memcpy(arg, tokens[i].tb, tokens[i].te - tokens[i].tb + 1);
            arg[tokens[i].te - tokens[i].tb + 1] = 0x00;

            lua_pushstring(L, arg);
            args++;
        }

        // In the stack there are:
        // nil
        // a reference to the module (if any)
        // a reference to the function
        // the function arguments

        // Call, and make room for 4 return values
        lua_pcall(L, args, 4, 0);

        // Check for errors
        if ((lua_type(L, 3) == LUA_TNIL) && (lua_type(L, 4) == LUA_TSTRING)) {
            const char *msg = lua_tostring(L, 4);
            if (msg) {
                printf("%s\r\n", msg);
            }

            // Clear stack
            lua_settop(L, 0);
        } else if ((lua_type(L, 1) == LUA_TNIL) && (lua_type(L, 3) == LUA_TSTRING))    {
            const char *msg = lua_tostring(L, 3);
            if (msg) {
                printf("%s\r\n", msg);
            }

            // Clear stack
            lua_settop(L, 0);
        } else if ((lua_type(L, 1) == LUA_TNIL) && (lua_type(L, 3) == LUA_TNUMBER)) {
            if (lua_isinteger(L,3)) {
                lua_Integer n = lua_tointeger(L, 3);

                printf(LUA_INTEGER_FMT "\r\n", n);
            } else {
                lua_Number n = lua_tonumber(L, 3);

                printf(LUA_NUMBER_FMT "\r\n", n);
            }

            // Clear stack
            lua_settop(L, 0);
        } else {
            // Clear stack
            lua_settop(L, 0);
        }

        *buffer = 0x00;
    }
    else if ((cindex < 0) && (itoken == 1)) {
        // Not a command - maybe it's a lua script?

        int tl = tokens[0].te - tokens[0].tb + 1;
        if ((tl > LUA_MAXINPUT - 1) || (tl > PATH_MAX)) {
            goto exit;
        }

        memcpy(arg, tokens[0].tb, tl);
        arg[tl] = 0x00;

        // for execution without "do " the file extension must be ".lua" or ".luac"
        if (
                (arg[tl-4] != '.' || arg[tl-3] != 'l' || arg[tl-2] != 'u' || arg[tl-1] != 'a') &&
                (arg[tl-5] != '.' || arg[tl-4] != 'l' || arg[tl-3] != 'u' || arg[tl-2] != 'a' || arg[tl-1] != 'c')
        ) {
            goto exit;
        }

        // check if a file system element with that name exists
        struct stat s;
        if (stat(arg, &s) < 0) {
            goto exit;
        }

        if (s.st_mode == S_IFDIR) {
            goto exit;
        }
        else if (s.st_mode == S_IFREG) {
            // It's a file

            char *argbuf = calloc(1, LUA_MAXINPUT);
            if (!argbuf) {
                goto exit;
            }

            get_args_from_shell_command(buffer, arg, argbuf);

            *buffer = 0x00;
            if (strlen(argbuf)) {
                strlcat(buffer,"assert(loadfile(\"",LUA_MAXINPUT);
                strlcat(buffer,arg, LUA_MAXINPUT); //script name
                strlcat(buffer,"\"))(", LUA_MAXINPUT);
                strlcat(buffer,argbuf, LUA_MAXINPUT); //script params
                strlcat(buffer,")", LUA_MAXINPUT);
            }
            else {
                strlcat(buffer,"dofile(\"",LUA_MAXINPUT);
                strlcat(buffer,arg, LUA_MAXINPUT); //script name
                strlcat(buffer,"\")", LUA_MAXINPUT);
            }

            if (argbuf)
                free(argbuf);
        }
    }

exit:
    if (arg)
        free(arg);

    if (tokens)
        free(tokens);
}

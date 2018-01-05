/*
 * Lua RTOS, shell
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
	{"mkdir", "os", "mkdir", 1, 0, "mkdir path"},
	{"more", "os", "more", 1, 0, "more filename"},
	{"mv", "os", "rename", 2, 0, "mv old new"},
	{"netstat", "net", "stat", 0, 0, NULL},
	{"ping", "net", "ping", 1, 0, "ping destination"},
	{"pwd", "os", "pwd", 0, 0, NULL},
	{"remove", "os", "remove", 1, 0, "remove filename"},
	{"rename", "os", "rename", 2, 0, "rename old new"},
	{"rm", "os", "remove", 1, 0, "rm filename"},
	{"unlink", "os", "remove", 1, 0, "unlink filename"},
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
	char arg[256];

	// Initialize an array for store tokens found in buffer
	struct {
		char *tb;
		char *te;
	} tokens[10];

	memset(tokens, 0, sizeof(tokens));

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
		return;
	}

	// Process command
	if (cindex >= 0) {
		// Check for mandatory arguments
		if ((itoken - 1) < command[cindex].mandatory) {
			if (command[cindex].usage) {
				printf("usage: %s\r\n", command[cindex].usage);
				*buffer = 0x00;
				return;
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
		} else if ((lua_type(L, 1) == LUA_TNIL) && (lua_type(L, 3) == LUA_TSTRING))	{
			const char *msg = lua_tostring(L, 3);
			if (msg) {
				printf("%s\r\n", msg);
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
		if ((tl > sizeof(arg) - 1) || (tl > PATH_MAX)) {
			return;
		}

		memcpy(arg, tokens[0].tb, tl);
		arg[tl] = 0x00;

		// for execution without "do " the file extension must be ".lua"
		if(arg[tl-4] != '.' || arg[tl-3] != 'l' || arg[tl-2] != 'u' || arg[tl-1] != 'a') {
			return;
		}

		// check if a file system element with that name exists
		struct stat s;
		if (stat(arg, &s) < 0) {
			return;
		}

		if (s.st_mode == S_IFDIR) {
			return;
		}
		else if (s.st_mode == S_IFREG) {
			// It's a file

			char argbuf[256];
			get_args_from_shell_command(buffer, arg, argbuf);

			*buffer = 0x00;
			if (strlen(argbuf)) {
				strlcat(buffer,"loadfile(\"",256);
				strlcat(buffer,arg, 256); //script name
				strlcat(buffer,"\")(", 256);
				strlcat(buffer,argbuf, 256); //script params
				strlcat(buffer,")", 256);
			}
			else {
				strlcat(buffer,"dofile(\"",256);
				strlcat(buffer,arg, 256); //script name
				strlcat(buffer,"\")", 256);
			}
			return;
		}
	}
	
}

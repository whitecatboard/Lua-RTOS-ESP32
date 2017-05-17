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
	const uint8_t error_pos;
} command_t;

static const command_t command[] = {
	{"cat", "os", "cat", 1, 0, "cat filename", 4},
	{"cd", "os", "cd", 0, 1, "cd path", 4},
	{"cp", "os", "cp", 2, 0, "cp from to", 4},
	{"dmesg", "os", "dmesg", 0, 0, NULL, 0},
	{"clear", "os", "clear", 0, 0, NULL, 0},
	{"edit", "os", "edit", 1, 0, "edit [filename]", 4},
	{"exit", "os", "exit", 0, 0, NULL, 0},
	{"ls", "os", "ls", 0, 1, "ls [pattern]", 4},
	{"dir", "os", "ls", 0, 1, "dir [pattern]", 4},
	{"mkdir", "os", "mkdir", 1, 0, "mkdir path", 4},
	{"more", "os", "more", 1, 0, "more filename", 4},
	{"mv", "os", "rename", 2, 0, "mv old new", 4},
	{"pwd", "os", "pwd", 0, 0, NULL, 0},
	{"remove", "os", "remove", 1, 0, "remove filename", 4},
	{"rename", "os", "rename", 2, 0, "rename old new", 4},
	{"rm", "os", "remove", 1, 0, "rm filename", 4},
	{"unlink", "os", "remove", 1, 0, "unlink filename", 4},
	{NULL, NULL, NULL, 0, 0, NULL, 0},
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
	while (*cbuffer) {
		cbuffer = lua_token_skip_spaces(cbuffer);
		tokens[itoken].tb = cbuffer;
		tokens[itoken].te = cbuffer;

		if (*cbuffer) {
			cbuffer = lua_token_skip_valid(cbuffer);
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

	// Process command
	if ((cindex < 0) && (itoken == 1)) {
		// Not a command
		// May be it's a lua script?
	    struct stat s;

	    int tl = tokens[0].te - tokens[0].tb + 1;
	    if (tl > sizeof(arg)) {
	    	return;
	    }

		memcpy(arg, tokens[0].tb, tl);
		arg[tokens[0].te - tokens[0].tb + 1] = 0x00;

	    if (stat(arg, &s) < 0) {
			return;
	    }

		if (s.st_mode == S_IFDIR) {
			return;
		} else if (s.st_mode == S_IFREG) {
			*buffer = 0x00;

			// It's a file
			strlcat(buffer,"dofile(\"",256);
			strlcat(buffer,arg, 256);
			strlcat(buffer,"\")", 256);

			return;
		}
	} else {
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
			lua_getglobal(L, command[cindex].module);
			lua_getfield(L, -1, command[cindex].function);

			// Prepare arguments
			int i, args = 0;

			for(i = 1; i < itoken; i++) {
				memcpy(arg, tokens[i].tb, tokens[i].te - tokens[i].tb + 1);
				arg[tokens[i].te - tokens[i].tb + 1] = 0x00;

			    lua_pushstring(L, arg);
			    args++;
			}

			// Call
		    lua_pcall(L, args, 4, 0);

		    // Check for errors
		    if (command[cindex].error_pos) {
				if (lua_type(L, command[cindex].error_pos) == LUA_TSTRING) {
					const char *msg = lua_tostring(L, command[cindex].error_pos);

					if (msg) {
						printf("%s\r\n", msg);
					}
			     }
		    }

			lua_settop(L, 0);
			*buffer = 0x00;
		}
	}
}

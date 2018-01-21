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
 * Lua RTOS console utility functions
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_CONSOLE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/console.h>
#include <sys/fcntl.h>
#include <sys/time.h>

void console_clear() {
	printf("\033[2J\033[1;1H");
}

void console_hide_cursor() {
    printf("\033[25h");
}

void console_show_cursor() {
    printf("\033[25l");
}

void console_size(int *rows, int *cols) {
	struct timeval start; // Start time
	struct timeval now;   // Current time
	int msectimeout = 100;
	char buf[6];
    char *cbuf;
    char c;

    // Save cursor position
    printf("\033[s");

    // Set cursor out of the screen
    printf("\033[999;999H");

    // Get cursor position
    printf("\033[6n");

    // Return to saved cursor position
    printf("\033[u");

	int flags = fcntl(fileno(stdin), F_GETFL, 0);
	fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK);

    // Skip scape sequence
    gettimeofday(&start, NULL);
    for(;;) {
    	if (read(fileno(stdin), &c, 1) == 1) {
			if (c == '\033') {
				break;
			}
    	}

		gettimeofday(&now, NULL);
		if ((now.tv_sec - start.tv_sec) * 1000 - (((now.tv_usec - start.tv_usec) + 500) / 1000) >= msectimeout) {
		   break;
		}
    }

    gettimeofday(&start, NULL);
    for(;;) {
    	if (read(fileno(stdin), &c, 1) == 1) {
			if (c == '[') {
				break;
			}
    	}

		gettimeofday(&now, NULL);
		if ((now.tv_sec - start.tv_sec) * 1000 - (((now.tv_usec - start.tv_usec) + 500) / 1000) >= msectimeout) {
		   break;
		}
    }

    // Read rows
    c = '\0';
    cbuf = buf;

    gettimeofday(&start, NULL);
    for(;;) {
    	if (read(fileno(stdin), &c, 1) == 1) {
			if (c == ';') {
				break;
			}
			*cbuf++ = c;
    	}

		gettimeofday(&now, NULL);
		if ((now.tv_sec - start.tv_sec) * 1000 - (((now.tv_usec - start.tv_usec) + 500) / 1000) >= msectimeout) {
		   break;
		}
    }

    *cbuf = '\0';

    if (*buf != '\0') {
        *rows = atoi(buf);
    }

    // Read cols
    c = '\0';
    cbuf = buf;

    gettimeofday(&start, NULL);
    for(;;) {
    	if (read(fileno(stdin), &c, 1) == 1) {
			if (c == 'R') {
				break;
			}
			*cbuf++ = c;
    	}

		gettimeofday(&now, NULL);
		if ((now.tv_sec - start.tv_sec) * 1000 - (((now.tv_usec - start.tv_usec) + 500) / 1000) >= msectimeout) {
		   break;
		}
    }

    fcntl(fileno(stdin), F_SETFL, flags);

    *cbuf = '\0';

    if (*buf != '\0') {
        *cols = atoi(buf);
    }
}

void console_gotoxy(int col, int line) {
    printf("\033[%d;%dH", line + 1, col + 1);
}

void console_statusline(const char *text1, const char *text2) {
    int rows = 0;
    int cols = 0;

    console_size(&rows, &cols);

    console_gotoxy(0, rows);
    printf("\033[1m\033[7m%s%s\033[K\033[0m", text1, text2);
}

void console_clearstatusline() {
    int rows = 0;
    int cols = 0;

    console_size(&rows, &cols);

    console_gotoxy(0, rows);
    printf("\033[K\033[0m");
}

void console_erase_eol() {
    printf("\033[K");
}

void console_erase_sol() {
	printf("\033[1K");
}

void console_erase_l() {
	printf("\033[2K");
}

#endif

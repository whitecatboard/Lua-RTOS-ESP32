/*
 * Lua RTOS, some console utility functions
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

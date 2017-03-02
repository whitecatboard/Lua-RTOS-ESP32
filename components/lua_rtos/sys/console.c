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

#include "luartos.h"

#if USE_CONSOLE

#include <stdio.h>
#include <stdlib.h>

#include <sys/console.h>
#include <drivers/uart.h>

void console_put(const char *c) {
    while (*c) {
        uart_write(CONSOLE_UART, *c++);
    }
}

void console_clear() {
	console_put("\033[2J\033[1;1H");
}

void console_hide_cursor() {
    printf("\033[25h");
}

void console_show_cursor() {
    printf("\033[25l");
}

void console_size(int *rows, int *cols) {
    char buf[6];
    char *cbuf;
    char c;

    // Save cursor position
    console_put("\033[s");

    // Set cursor out of the screen
    console_put("\033[999;999H");

    // Get cursor position
    console_put("\033[6n");

    // Return to saved cursor position
    console_put("\033[u");

    // Skip scape sequence
    while (uart_read(CONSOLE_UART, &c, 100) && (c != '\033')) {
	}

    while (uart_read(CONSOLE_UART, &c, 100) && (c != '[')) {
	}

    // Read rows
    c = '\0';
    cbuf = buf;
    while (uart_read(CONSOLE_UART, &c, 100) && (c != ';')) {
	    *cbuf++ = c;
	}
    *cbuf = '\0';

    if (*buf != '\0') {
        *rows = atoi(buf);
    }

    // Read cols
    c = '\0';
    cbuf = buf;
    while (uart_read(CONSOLE_UART, &c, 100) && (c != 'R')) {
		*cbuf++ = c;
	}
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
    console_put("\033[K");
}

void console_erase_sol() {
    console_put("\033[1K");
}

void console_erase_l() {
    console_put("\033[2K");
}

#endif

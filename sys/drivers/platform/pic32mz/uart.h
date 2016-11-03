/*
 * Lua RTOS, UART driver
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

#ifndef UART_H
#define	UART_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include <stdint.h>

#define NUART 6

void uart_init(u8_t unit, u32_t brg, u32_t mode, u32_t qs);
void    uart_init_interrupts(u8_t unit);
void    uart_write(u8_t unit, char byte);
void    uart_writes(u8_t unit, char *s);
u8_t    uart_read(u8_t unit, char *c, uint32_t timeout);
uint8_t uart_reads(u8_t unit, char *buff, u8_t crlf, uint32_t timeout);
uint8_t uart_wait_response(u8_t unit, char *command, uint8_t echo, char *ret, uint8_t substring, uint32_t timeout, int nargs, ...);
uint8_t uart_send_command(u8_t unit, char *command, uint8_t echo, uint8_t crlf, char *ret, uint8_t substring, uint32_t timeout, int nargs, ...);
void    uart_consume(u8_t unit);
char   *uart_name(u8_t unit);
QueueHandle_t *uart_get_queue(u8_t unit);
void uart_debug(int unit, int debug);
int uart_get_br(int unit);
int uart_inited(int unit);
void uart_intr_rx(u8_t unit);
void uart_intr_tx(u8_t unit);
void uart_intr_er(u8_t unit);

#define uart0_swap()
#define uart0_default()

#endif


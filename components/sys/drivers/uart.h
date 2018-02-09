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
 * Lua RTOS, UART driver
 *
 */

/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __UART_H__
#define __UART_H__

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "rom/uart.h"
#include "rom/ets_sys.h"
#include "soc/uart_reg.h"
#include "soc/io_mux_reg.h"

#include <stdint.h>
#include <pthread.h>
#include <sys/driver.h>

struct uart {
    uint8_t          flags;
    QueueHandle_t    q;         // RX queue
    uint16_t         qs;        // Queue size
    uint32_t         brg;       // Baud rate
    pthread_mutex_t  mtx;		// Mutex
    int8_t           rx;
    int8_t           tx;
};

// Resources used by the UART
typedef struct {
	uint8_t rx;
	uint8_t tx;
} uart_resources_t;

// Number of UART units
#define NUART 3

// UART errors
#define UART_ERR_CANT_INIT                (DRIVER_EXCEPTION_BASE(UART_DRIVER_ID) |  0)
#define UART_ERR_INVALID_UNIT			  (DRIVER_EXCEPTION_BASE(UART_DRIVER_ID) |  1)
#define UART_ERR_INVALID_DATA_BITS		  (DRIVER_EXCEPTION_BASE(UART_DRIVER_ID) |  2)
#define UART_ERR_INVALID_PARITY			  (DRIVER_EXCEPTION_BASE(UART_DRIVER_ID) |  3)
#define UART_ERR_INVALID_STOP_BITS		  (DRIVER_EXCEPTION_BASE(UART_DRIVER_ID) |  4)
#define UART_ERR_NOT_ENOUGH_MEMORY		  (DRIVER_EXCEPTION_BASE(UART_DRIVER_ID) |  5)
#define UART_ERR_IS_NOT_SETUP 			  (DRIVER_EXCEPTION_BASE(UART_DRIVER_ID) |  6)
#define UART_ERR_PIN_NOT_ALLOWED		  (DRIVER_EXCEPTION_BASE(UART_DRIVER_ID) |  7)
#define UART_ERR_CANNOT_CHANGE_PINMAP	  (DRIVER_EXCEPTION_BASE(UART_DRIVER_ID) |  8)

// Flags
#define UART_FLAG_WRITE 0x01
#define UART_FLAG_READ  0x02
#define UART_FLAG_ALL (UART_FLAG_WRITE | UART_FLAG_READ)

#define ETS_UART_INTR_ENABLE()  _xt_isr_unmask(1 << ETS_UART_INUM)
#define ETS_UART_INTR_DISABLE() _xt_isr_mask(1 << ETS_UART_INUM)
#define UART_INTR_MASK          0x1ff

#define wait_tx_empty(unit) \
while ((READ_PERI_REG(UART_STATUS_REG(unit)) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);delay(1);

driver_error_t *uart_init(int8_t unit, uint32_t brg, uint8_t databits, uint8_t parity, uint8_t stop_bits, uint8_t flags, uint32_t qs);
driver_error_t *uart_setup_interrupts(int8_t unit);
driver_error_t *uart_consume(int8_t unit);
driver_error_t *uart_lock(int unit);
driver_error_t *uart_unlock(int unit);

void uart_ll_lock(int unit);
void uart_ll_unlock(int unit);
void uart_ll_set_raw(uint8_t raw);

driver_error_t *uart_pin_map(int unit, int rx, int tx);
void     uart_write(int8_t unit, char byte);
void     uart_writes(int8_t unit, char *s);
uint8_t uart_read(int8_t unit, char *c, uint32_t timeout);
uint8_t  uart_reads(int8_t unit, char *buff, uint8_t crlf, uint32_t timeout);
uint8_t  uart_wait_response(int8_t unit, char *command, uint8_t echo, char *ret, uint8_t substring, uint32_t timeout, int nargs, ...);
uint8_t  uart_send_command(int8_t unit, char *command, uint8_t echo, uint8_t crlf, char *ret, uint8_t substring, uint32_t timeout, int nargs, ...);
const char  *uart_name(int8_t unit);
int      uart_get_br(int unit);
int      uart_is_setup(int unit);
void     uart_stop(int unit);
QueueHandle_t *uart_get_queue(int8_t unit);
driver_error_t *uart_lock_resources(int unit, uint8_t flags, void *resources);

#endif

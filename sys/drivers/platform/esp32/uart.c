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

#include "whitecat.h"

#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <pthread.h>

#include <sys/drivers/uart.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/console.h>
#include <sys/drivers/cpu.h>
#include <sys/syslog.h>
#include <sys/status.h>

#include <esp_types.h>
#include "esp_err.h"
#include "esp_intr.h"
#include "xtensa_api.h"
#include "soc/soc.h"


// Flags for determine some UART states
#define UART_FLAG_INIT		(1 << 1)
#define UART_FLAG_IRQ_INIT	(1 << 2)

#define ETS_UART_INUM  5
#define UART_INTR_SOURCE(u) ((u==0)?ETS_UART0_INTR_SOURCE:( (u==1)?ETS_UART1_INTR_SOURCE:((u==2)?ETS_UART2_INTR_SOURCE:0)))

// Names for the uarts
static const char *names[] = {
	"uart1",
	"uart2",
	"uart3",
};

struct uart {
    u8_t          flags;
    QueueHandle_t q;         // RX queue
    u16_t         qs;        // Queue size
    u32_t         brg;       // Queue size
};

struct uart uart[NUART] = {
    {
        0, NULL, 0, 115200
    },
    {
        0, NULL, 0, 115200
    },
    {
        0, NULL, 0, 115200
    },
};

void uart_update_params(u8_t unit, UartBautRate brg, UartBitsNum4Char data, UartParityMode parity, UartStopBitsNum stop) {
	wait_tx_empty(unit);

	uart_div_modify(unit++, (APB_CLK_FREQ << 4) / brg);

    WRITE_PERI_REG(UART_CONF0_REG(unit),
                   ((parity == NONE_BITS) ? 0x0 : (UART_PARITY_EN | parity))
                   | (stop << UART_STOP_BIT_NUM_S)
                   | (data << UART_BIT_NUM_S));
}

void uart_pin_config(u8_t unit, u8_t *rx, u8_t *tx) {
	wait_tx_empty(0);
	wait_tx_empty(1);

	switch (unit) {
		case 0:
			// Enable UTX0
			PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD_U0TXD);

			// Enable U0RX
	        PIN_PULLUP_EN(PERIPHS_IO_MUX_U0RXD_U);
	        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD_U0RXD);

			if (rx) *rx = PIN_GPIO3;
			if (tx) *tx = PIN_GPIO1;
			
			break;
						
		case 1:
			// Enable U1TX
			PIN_PULLUP_DIS(PERIPHS_IO_MUX_SD_DATA3_U);
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA3_U, FUNC_SD_DATA3_U1TXD);

			// Enable U1RX
	        PIN_PULLUP_EN(PERIPHS_IO_MUX_SD_DATA2_U);
	        PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA2_U, FUNC_SD_DATA2_U1RXD);

			if (rx) *rx = PIN_GPIO9;
			if (tx) *tx = PIN_GPIO10;

			break;

		case 2:
			// Enable U2TX
			PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO17_U);
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO17_U, FUNC_GPIO17_U2TXD);

			// Enable U2RX
	        PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO16_U);
	        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO16_U, FUNC_GPIO16_U2RXD);

			if (rx) *rx = PIN_GPIO16;
			if (tx) *tx = PIN_GPIO17;

			break;
	}
}

static int queue_byte(u8_t unit, u8_t byte, int *signal) {
	*signal = 0;

    if (unit == CONSOLE_UART - 1) {
        if (byte == 0x04) {
            if (!status_get(STATUS_LUA_RUNNING)) {
                uart_writes(CONSOLE_UART, "Lua RTOS-booting\r\n");
            } else {
                uart_writes(CONSOLE_UART, "Lua RTOS-running\r\n");
            }
            
    		status_set(STATUS_LUA_ABORT_BOOT_SCRIPTS);
			
            return 0;
        } else if (byte == 0x03) {
        	if (status_get(STATUS_LUA_RUNNING)) {
				*signal = SIGINT;
				if (_pthread_has_signal(*signal)) {
					return 0;
				}

				return 1;
        	} else {
        		return 0;
        	}
        }
    }
	
	if (status_get(STATUS_LUA_RUNNING)) {
		return 1;
	} else {
		return 0;
	}
}

void  uart_rx_intr_handler(void *para) {
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    u32_t uart_intr_status = 0;
	u8_t byte;
	int signal = 0;
	int unit = 0;

	for(;unit < NUART;unit++) {
		uart_intr_status = READ_PERI_REG(UART_INT_ST_REG(unit)) ;

	    while (uart_intr_status != 0x0) {
	        if (UART_FRM_ERR_INT_ST == (uart_intr_status & UART_FRM_ERR_INT_ST)) {
	            WRITE_PERI_REG(UART_INT_CLR_REG(unit), UART_FRM_ERR_INT_CLR);
	        } else if (UART_RXFIFO_FULL_INT_ST == (uart_intr_status & UART_RXFIFO_FULL_INT_ST)) {
	            while ((READ_PERI_REG(UART_STATUS_REG(unit)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT) {
					byte = READ_PERI_REG(UART_FIFO_REG(unit)) & 0xFF;
					if (queue_byte(unit, byte, &signal)) {
			            // Put byte to UART queue
			            xQueueSendFromISR(uart[unit].q, &byte, &xHigherPriorityTaskWoken);
					} else {
						if (signal) {
							_pthread_queue_signal(signal);
						}
					}
	            }

	            WRITE_PERI_REG(UART_INT_CLR_REG(unit), UART_RXFIFO_FULL_INT_CLR);
	        } else if (UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST)) {
	            while ((READ_PERI_REG(UART_STATUS_REG(unit)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT) {
					byte = READ_PERI_REG(UART_FIFO_REG(unit)) & 0xFF;
					if (queue_byte(unit, byte, &signal)) {
			            // Put byte to UART queue
			            xQueueSendFromISR(uart[unit].q, &byte, &xHigherPriorityTaskWoken);
					} else {
						if (signal) {
							_pthread_queue_signal(signal);
						}
					}
	            }

	            WRITE_PERI_REG(UART_INT_CLR_REG(unit), UART_RXFIFO_TOUT_INT_CLR);
	        } else if (UART_TXFIFO_EMPTY_INT_ST == (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST)) {
	            WRITE_PERI_REG(UART_INT_CLR_REG(unit), UART_TXFIFO_EMPTY_INT_CLR);
	            CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(unit), UART_TXFIFO_EMPTY_INT_ENA);
	        }

	        uart_intr_status = READ_PERI_REG(UART_INT_ST_REG(unit)) ;
	    }
	}

	if (xHigherPriorityTaskWoken) {
		portYIELD_FROM_ISR();
	}
}

// Inits UART
// UART is configured by setting baud rate, 8N1
// Interrupts are not enabled in this function
void uart_init(u8_t unit, u32_t brg, u32_t mode, u32_t qs) {
	u8_t rx, tx;
	
    unit--;
	
	// If requestes queue size is greater than current size, delete queue and create
	// a new one
    if (qs > uart[unit].qs) {
		if (uart[unit].q) {
			vQueueDelete(uart[unit].q);
		}
		
		uart[unit].q  = xQueueCreate(qs, sizeof(u8_t));
	}

	uart_pin_config(unit, &rx, &tx);

	//if (!uart0_swaped) {
	//	syslog(LOG_INFO, "%s: at pins rx=%s/tx=%s", names[unit], cpu_pin_name(rx), cpu_pin_name(tx));
	//	syslog(LOG_INFO, "%s: speed %d bauds",names[unit], brg);  
	//}  

	uart_update_params(unit, brg, EIGHT_BITS, NONE_BITS, ONE_STOP_BIT);
	
    uart[unit].brg = brg; 
    uart[unit].qs  = qs; 

    uart[unit].flags |= UART_FLAG_INIT;
}

// Enable UART interrupts
void uart_init_interrupts(u8_t unit) {
    unit--;

    if (uart[unit].flags & UART_FLAG_IRQ_INIT) {
        return;
    }

    u32_t reg_val = 0;
	u32_t mask = UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA | UART_RXFIFO_FULL_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA;
		
	ESP_INTR_DISABLE(ETS_UART_INUM);

	// Update CONF1 register
    reg_val = READ_PERI_REG(UART_CONF1_REG(unit)) & ~((UART_RX_FLOW_THRHD << UART_RX_FLOW_THRHD_S) | UART_RX_FLOW_EN) ;

    reg_val |= ((mask & UART_RXFIFO_TOUT_INT_ENA) ?
                (((2 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S) | UART_RX_TOUT_EN) : 0);

    reg_val |= ((mask & UART_RXFIFO_FULL_INT_ENA) ?
                ((10 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) : 0);

    reg_val |= ((mask & UART_TXFIFO_EMPTY_INT_ENA) ?
                ((20 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S) : 0);

    WRITE_PERI_REG(UART_CONF1_REG(unit), reg_val);

	// Update INT_ENA register
    WRITE_PERI_REG(UART_INT_ENA_REG(unit), mask);
	

	intr_matrix_set(xPortGetCoreID(), UART_INTR_SOURCE(unit), ETS_UART_INUM);
	xt_set_interrupt_handler(ETS_UART_INUM, uart_rx_intr_handler, NULL);
	ESP_INTR_ENABLE(ETS_UART_INUM);
	
		//syslog(LOG_INFO, "%s: interrupts %d/%d/%d",names[unit].name,
        //      uart[unit].irqs.rx,uart[unit].irqs.tx,uart[unit].irqs.er);
}

// Writes a byte to the UART
void uart_write(u8_t unit, char byte) {
    unit--;
	 
    while (((READ_PERI_REG(UART_STATUS_REG(unit)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S)) >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) >= 126);
    WRITE_PERI_REG(UART_FIFO_REG(unit), byte);
}

// Writes a null-terminated string to the UART
void uart_writes(u8_t unit, char *s) {
    unit--;

    while (*s) {
	    while (((READ_PERI_REG(UART_STATUS_REG(unit)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S)) >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) >= 126);
	    WRITE_PERI_REG(UART_FIFO_REG(unit) , *s++);
   }
}

// Reads a byte from uart
u8_t uart_read(u8_t unit, char *c, u32_t timeout) {
    unit--;
	
    if (timeout != portMAX_DELAY) {
        timeout = timeout / portTICK_PERIOD_MS;
    }
	
    if (xQueueReceive(uart[unit].q, c, (TickType_t)(timeout)) == pdTRUE) {
        return 1;
    } else {
        return 0;
    }
}

// Consume all received bytes, and do not nothing with them
void uart_consume(u8_t unit) {
    char tmp;
    
    while(uart_read(unit,&tmp,1));
} 

// Reads a string from the UART, ended by the CR + LF character
u8_t uart_reads(u8_t unit, char *buff, u8_t crlf, uint32_t timeout) {
    char c;

    for (;;) {
        if (uart_read(unit, &c, timeout)) {
            if (c == '\0') {
                return 1;
            } else if (c == '\n') {
                *buff = 0;
                return 1;
            } else {
                if ((c == '\r') && !crlf) {
                    *buff = 0;
                    return 1;
                } else {
                    if (c != '\r') {
                        *buff++ = c;
                    }
                }
            }
        } else {
            return 0;
        }
    }

    return 0;
}

// Read from the UART and waits for a response
static u8_t _uart_wait_response(u8_t unit, char *command, u8_t echo, char *ret, u8_t substring, uint32_t timeout, int nargs, va_list pargs) {
    int ok = 1;

    va_list args;
    
    char buffer[80];
    char *arg;

    // Test if we receive an echo of the command sended
    if ((command != NULL) && (echo)) {
        if (uart_reads(unit,buffer, 1, timeout)) {
            ok = (strcmp(buffer, command) == 0);
        } else {
            ok = 0;
        }
    }

    if (ok && nargs > 0) {
        ok = 0;

        // Read until we received expected response
        while (!ok) {
            if (uart_reads(unit,buffer, 1, timeout)) {
                args = pargs;

                int i;
                for (i = 0; i < nargs; i++) {
                    arg = va_arg(args, char *);
                    if (!substring) {
                        ok = ((strcmp(buffer, arg) == 0) || (strcmp(buffer, "ERROR") == 0));
                    } else {
                        ok = ((strstr(buffer, arg) != 0) || (strcmp(buffer, "ERROR") == 0));
                    }

                    if (ok) {
                        // If we expected for a return, copy
                        if (ret != NULL) {
                            strcpy(ret, buffer);
                        }

                        break;
                    }
                }

                if (strcmp(buffer, "ERROR") == 0) {
                    ok = 0;

                    break;
                }
            } else {
                ok = 0;
                break;
            }
        }
    }

    return ok;
}

// Read from the UART and waits for a response
u8_t uart_wait_response(u8_t unit, char *command, u8_t echo, char *ret, u8_t substring, uint32_t timeout, int nargs, ...) {
    va_list pargs;

    va_start(pargs, nargs);

    u8_t ok = _uart_wait_response(unit, command, echo, ret, substring, timeout, nargs, pargs);

    va_end(pargs);

    return ok;
}

// Sends a command to a device connected to the UART and waits for a response
u8_t uart_send_command(u8_t unit, char *command, u8_t echo, u8_t crlf, char *ret, u8_t substring, uint32_t timeout, int nargs, ...) {
    u8_t ok = 0;

    uart_writes(unit,command);
    if (crlf) {
        uart_writes(unit,"\r\n");
    }

    va_list pargs;
    va_start(pargs, nargs);


    ok = _uart_wait_response(unit, command, echo, ret, substring, timeout, nargs, pargs);

    va_end(pargs);

    return ok;
}

// Gets the UART name
const char *uart_name(u8_t unit) {
    return names[unit - 1];
}

// Gets the UART queue
QueueHandle_t *uart_get_queue(u8_t unit) {
    unit--;

    return  uart[unit].q;
}

int uart_get_br(int unit) {
//    int divisor;
//    unit--;

//    reg = uart[unit].regs;
//    divisor = reg->brg;

//    return ((double)PBCLK2_HZ / (double)(16 * (divisor + 1)));
	return 0;
}

int uart_inited(int unit) {
    unit--;
    return ((uart[unit].flags & UART_FLAG_INIT) && (uart[unit].flags & UART_FLAG_IRQ_INIT));
}

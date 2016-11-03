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
#include <stdarg.h>
#include <signal.h>
#include <pthread.h>

#include <sys/drivers/uart.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/console.h>
#include <sys/drivers/cpu.h>
#include <sys/syslog.h>
#include <sys/status.h>

// Flags for determine some UART states
#define UART_FLAG_INIT		(1 << 1)
#define UART_FLAG_IRQ_INIT	(1 << 2)

// Mask for determine if UART output is the default, or
// the alternate (when swap is made)
#define UART_OUTPUT_NONE 		 0
#define UART_OUTPUT_DEFAULT 	(1 << 0)
#define UART_OUTPUT_ALTERNATE 	(1 << 1)

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
	u8_t		  phys;
};

struct uart uart[NUART] = {
    {
        0, NULL, 0, 115200, 0
    },
    {
        0, NULL, 0, 115200, 1
    },
    {
        0, NULL, 0, 115200, 0
    },
};

// Is UART0 swaped?
static int uart0_swaped = -1;

void uart0_swap();
void uart0_default();
	
void uart_update_params(u8_t unit, UART_BautRate brg, UART_WordLength data, UART_ParityMode parity, UART_StopBits stop) {
	wait_tx_empty(0);
	wait_tx_empty(1);

    sdk_uart_div_modify(unit, UART_CLK_FREQ / brg);

    WRITE_PERI_REG(UART_CONF0(unit),
                   ((parity == USART_Parity_None) ? 0x0 : (UART_PARITY_EN | parity))
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
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
	
			// Enable U0RX
	        PIN_PULLUP_EN(PERIPHS_IO_MUX_U0RXD_U);
	        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD);

			// Disable U1TX
	   	 	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);					

			if (rx) *rx = PIN_GPIO3;
			if (tx) *tx = PIN_GPIO1;
			
			break;
						
		case 1:
			// Disable UTX0
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);

			// Enable U1TX
		    PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U);
		    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);	
							

			if (rx) *rx = PIN_NOPIN;
			if (tx) *tx = PIN_GPIO2;

			break;

		case 2:
			// Disable UTX0
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);
		
			// Enable U0TX*
		    PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTDO_U);
		    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_UART0_RTS);

			// Enable U0RX*
		    PIN_PULLUP_EN(PERIPHS_IO_MUX_MTCK_U);
		    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);

			// Enable U1TX
		    PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U);
		    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);					

			if (rx) *rx = PIN_GPIO13;
			if (tx) *tx = PIN_GPIO15;
			
			break;
	}
	
	// Reset FIFO
	//SET_PERI_REG_MASK(UART_CONF0(unit), UART_RXFIFO_RST | UART_TXFIFO_RST);
	//CLEAR_PERI_REG_MASK(UART_CONF0(unit), UART_RXFIFO_RST | UART_TXFIFO_RST);
}

void uart0_swap() {
	enter_critical_section();
	
	if (uart0_swaped == 1) {
		exit_critical_section();
		return;
	}
	
	console_swap();	

	uart_pin_config(2, NULL, NULL);
	uart_update_params(0, uart[2].brg, UART_WordLength_8b, USART_Parity_None, USART_StopBits_1);
    IOSWAP |= (1 << IOSWAPU0);
		
	uart0_swaped = 1;
	
	exit_critical_section();
}

void uart0_default() {
	enter_critical_section();
	
	if (uart0_swaped == 0) {
		exit_critical_section();
		return;
	}
	
	console_default();

	uart_pin_config(0, NULL, NULL);
	uart_update_params(0, uart[0].brg, UART_WordLength_8b, USART_Parity_None, USART_StopBits_1);
    IOSWAP &= ~(1 << IOSWAPU0);

	uart0_swaped = 0;
	
	exit_critical_section();
}

void UART_IntrConfig(UART_Port uart_no,  UART_IntrConfTypeDef *pUARTIntrConf) {
    u32_t reg_val = 0;

    WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_INTR_MASK);
    reg_val = READ_PERI_REG(UART_CONF1(uart_no)) & ~((UART_RX_FLOW_THRHD << UART_RX_FLOW_THRHD_S) | UART_RX_FLOW_EN) ;

    reg_val |= ((pUARTIntrConf->UART_IntrEnMask & UART_RXFIFO_TOUT_INT_ENA) ?
                ((((pUARTIntrConf->UART_RX_TimeOutIntrThresh)&UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S) | UART_RX_TOUT_EN) : 0);

    reg_val |= ((pUARTIntrConf->UART_IntrEnMask & UART_RXFIFO_FULL_INT_ENA) ?
                (((pUARTIntrConf->UART_RX_FifoFullIntrThresh)&UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) : 0);

    reg_val |= ((pUARTIntrConf->UART_IntrEnMask & UART_TXFIFO_EMPTY_INT_ENA) ?
                (((pUARTIntrConf->UART_TX_FifoEmptyIntrThresh)&UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S) : 0);

    WRITE_PERI_REG(UART_CONF1(uart_no), reg_val);
    CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_INTR_MASK);
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), pUARTIntrConf->UART_IntrEnMask);
}

static int queue_byte(u8_t unit, u8_t byte, int *signal) {
    if (unit == CONSOLE_UART - 1) {
        if (byte == 0x04) {
            if (!status_get(STATUS_LUA_RUNNING)) {
                uart_writes(CONSOLE_UART, "Lua RTOS-booting\r\n");
            } else {
                uart_writes(CONSOLE_UART, "Lua RTOS-running\r\n");
            }
            
    		status_set(STATUS_LUA_ABORT_BOOT_SCRIPTS);
			*signal = 0;
			
            return 0;
        } else if (byte == 0x03) {
            *signal = SIGINT;
            if (_pthread_has_signal(*signal)) {
            	return 0;
            }
			
			return 1;
        }
    }
	
	return 1;	
}

static void uart_rx_intr_handler(void) {
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
	u8_t byte;
	int uart_no = 0;
	int signal = 0;
		
	u32_t uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no)) ;

    while (uart_intr_status != 0x0) {
        if (UART_FRM_ERR_INT_ST == (uart_intr_status & UART_FRM_ERR_INT_ST)) {
			
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_FRM_ERR_INT_CLR);
        } else if (UART_RXFIFO_FULL_INT_ST == (uart_intr_status & UART_RXFIFO_FULL_INT_ST)) {
            while ((READ_PERI_REG(UART_STATUS(uart_no)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT) {
				byte = READ_PERI_REG(UART_FIFO(uart_no)) & 0xFF;
				if (queue_byte(uart_no, byte, &signal)) {
		            // Put byte to UART queue
		            xQueueSendFromISR(uart[(uart0_swaped==1?2:0)].q, &byte, &xHigherPriorityTaskWoken);		              			
				} else {
					if (signal) {
						_pthread_queue_signal(signal);
					}
				}
            }

            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_RXFIFO_FULL_INT_CLR);
        } else if (UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST)) {
            while ((READ_PERI_REG(UART_STATUS(uart_no)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT) {
				byte = READ_PERI_REG(UART_FIFO(uart_no)) & 0xFF;
				if (queue_byte(uart_no, byte, &signal)) {
		            // Put byte to UART queue
		            xQueueSendFromISR(uart[(uart0_swaped==1?2:0)].q, &byte, &xHigherPriorityTaskWoken);		              			
				} else {
					if (signal) {
						_pthread_queue_signal(signal);
					}
				}
            }

            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_RXFIFO_TOUT_INT_CLR);
        } else if (UART_TXFIFO_EMPTY_INT_ST == (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST)) {
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_TXFIFO_EMPTY_INT_CLR);
            CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_TXFIFO_EMPTY_INT_ENA);
        }

        uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no)) ;
    }
	
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
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
	
	uart_pin_config(uart[unit].phys, &rx, &tx);

	if (!uart0_swaped) {
		syslog(LOG_INFO, "%s: at pins rx=%s/tx=%s", names[unit], cpu_pin_name(rx), cpu_pin_name(tx));
		syslog(LOG_INFO, "%s: speed %d bauds",names[unit], brg);  
	}  

	uart_update_params(uart[unit].phys, brg, UART_WordLength_8b, USART_Parity_None, USART_StopBits_1);
	
    uart[unit].brg = brg; 
    uart[unit].qs  = qs; 

    uart[unit].flags |= UART_FLAG_INIT;
}

// Enable UART interrupts
void uart_init_interrupts(u8_t unit) {
    UART_IntrConfTypeDef uart_intr;

    unit--;

    if (uart[unit].flags & UART_FLAG_IRQ_INIT) {
        return;
    }

	// Configure interrupt
    uart_intr.UART_IntrEnMask = UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA | UART_RXFIFO_FULL_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA;
    uart_intr.UART_RX_FifoFullIntrThresh = 10;
    uart_intr.UART_RX_TimeOutIntrThresh = 2;
    uart_intr.UART_TX_FifoEmptyIntrThresh = 20;

    UART_IntrConfig(unit, &uart_intr);

	_xt_isr_attach(ETS_UART_INUM, uart_rx_intr_handler);
	
    ETS_UART_INTR_ENABLE();

	int i;
	for(i=0;i<NUART;i++) {
	    uart[i].flags |= UART_FLAG_IRQ_INIT;		
	}

		//syslog(LOG_INFO, "%s: interrupts %d/%d/%d",names[unit].name,
        //      uart[unit].irqs.rx,uart[unit].irqs.tx,uart[unit].irqs.er);
}

// Writes a byte to the UART
void uart_write(u8_t unit, char byte) {
    unit--;

    while (((READ_PERI_REG(UART_STATUS(uart[unit].phys)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S)) >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) >= 126);
    WRITE_PERI_REG(UART_FIFO(uart[unit].phys), byte);
}

// Writes a null-terminated string to the UART
void uart_writes(u8_t unit, char *s) {
    unit--;

    while (*s) {
	    while (((READ_PERI_REG(UART_STATUS(uart[unit].phys)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S)) >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) >= 126);
	    WRITE_PERI_REG(UART_FIFO(uart[unit].phys) , *s++);
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

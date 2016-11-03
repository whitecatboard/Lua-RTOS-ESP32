/*
 * UART driver for pic32.
 *
 * Copyright (C) 2014 Serge Vakulenko, <serge@vak.ru>
 * 
 * 
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

#include "whitecat.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

#include <machine/pic32mz.h>
#include <pthread.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/uart.h>

#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <sys/syslog.h>
#include <sys/status.h>

#define DEBUG 0

#define UART_IRQ_INIT(name) { \
        name##E,  \
        name##RX, \
        name##TX, \
        1 << (name##E  & 31), \
        1 << (name##RX & 31), \
        1 << (name##TX & 31), \
        0x1f << (8 * (name##E  & 0x03)), \
        0x1f << (8 * (name##RX & 0x03)), \
        0x1f << (8 * (name##TX & 0x03)), \
        &IECSET(name##E  >> 5), \
        &IECSET(name##RX >> 5), \
        &IECSET(name##TX >> 5), \
        &IFSCLR(name##E  >> 5), \
        &IFSCLR(name##RX >> 5), \
        &IFSCLR(name##TX >> 5), \
        &IPCCLR(name##E  >> 2), \
        &IPCCLR(name##RX >> 2), \
        &IPCCLR(name##TX >> 2), \
        &IPCSET(name##E  >> 2), \
        &IPCSET(name##RX >> 2), \
        &IPCSET(name##TX >> 2), \
    }

struct uart_irq {
    int er;                             /* Receive error interrupt number */
    int rx;                             /* Receive interrupt number */
    int tx;                             /* Transmit interrupt number */
    unsigned er_mask;                   /* Receive error irq bitmask */
    unsigned rx_mask;                   /* Receive irq bitmask */
    unsigned tx_mask;                   /* Transmit irq bitmask */
    unsigned ipc_er_mask;               /* Receive error irq bitmask */
    unsigned ipc_rx_mask;               /* Receive irq bitmask */
    unsigned ipc_tx_mask;               /* Transmit irq bitmask */
    volatile unsigned *enable_er_intr;  /* IECSET pointer for error */
    volatile unsigned *enable_rx_intr;  /* IECSET pointer for receive */
    volatile unsigned *enable_tx_intr;  /* IECSET pointer for transmit */
    volatile unsigned *clear_er_intr;   /* IFSCLR pointer for receive error */
    volatile unsigned *clear_rx_intr;   /* IFSCLR pointer for receive */
    volatile unsigned *clear_tx_intr;   /* IFSCLR pointer for transmit */
    volatile unsigned *clear_er_ipc;
    volatile unsigned *clear_rx_ipc;
    volatile unsigned *clear_tx_ipc;
    volatile unsigned *enable_er_ipc;
    volatile unsigned *enable_rx_ipc;
    volatile unsigned *enable_tx_ipc;
};

struct uart_ipc_reg {
    volatile unsigned ipc;
    volatile unsigned clr;
    volatile unsigned set;
    volatile unsigned inv;
};

struct uart_ipc_mask {
    unsigned pos;
    unsigned mask;
};

struct uart_reg {
    volatile unsigned mode;		/* Mode */
    volatile unsigned modeclr;
    volatile unsigned modeset;
    volatile unsigned modeinv;
    volatile unsigned sta;		/* Status and control */
    volatile unsigned staclr;
    volatile unsigned staset;
    volatile unsigned stainv;
    volatile unsigned txreg;            /* Transmit */
    volatile unsigned unused1;
    volatile unsigned unused2;
    volatile unsigned unused3;
    volatile unsigned rxreg;            /* Receive */
    volatile unsigned unused4;
    volatile unsigned unused5;
    volatile unsigned unused6;
    volatile unsigned brg;		/* Baud rate */
    volatile unsigned brgclr;
    volatile unsigned brgset;
    volatile unsigned brginv;
};

struct uart {
    struct uart_reg      *regs;
    struct uart_irq       irqs;
    char                 *name;
    u8_t                  init;
    u8_t                  irq_init;
    u8_t                  console;
    QueueHandle_t         q;        // RX queue
    u32_t                 qs;       // Queue size
    u8_t                  debug;
};

struct uart uart[NUART] = {
    {
        (struct uart_reg*) &U1MODE,
        UART_IRQ_INIT(PIC32_IRQ_U1),
        "uart1",0,0, CONSOLE_UART == 1, NULL, 0, 0
    },
    {
        (struct uart_reg*) &U2MODE,
        UART_IRQ_INIT(PIC32_IRQ_U2),
        "uart2",0,0, CONSOLE_UART == 2, NULL, 0, 0
    },
    {
        (struct uart_reg*) &U3MODE,
        UART_IRQ_INIT(PIC32_IRQ_U3),
        "uart3",0,0, CONSOLE_UART == 3, NULL, 0, 0
    },
    {
        (struct uart_reg*) &U4MODE,
        UART_IRQ_INIT(PIC32_IRQ_U4),
        "uart4",0,0, CONSOLE_UART == 4, NULL, 0, 0
    },
    {
        (struct uart_reg*) &U5MODE,
        UART_IRQ_INIT(PIC32_IRQ_U5),
        "uart5",0,0, CONSOLE_UART == 5, NULL, 0, 0
    },
    {
        (struct uart_reg*) &U6MODE,
        UART_IRQ_INIT(PIC32_IRQ_U6),
        "uart6",0,0, CONSOLE_UART == 6, NULL, 0, 0
    }
};

extern const char pin_name;

/*
 * Assign UxRX signal to specified pin.
 */
static void assign_rx(int channel, int pin)
{
    switch (channel) {
    case 0: U1RXR = gpio_input_map1(pin); break;
    case 1: U2RXR = gpio_input_map3(pin); break;
    case 2: U3RXR = gpio_input_map2(pin); break;
    case 3: U4RXR = gpio_input_map4(pin); break;
    case 4: U5RXR = gpio_input_map1(pin); break;
    case 5: U6RXR = gpio_input_map4(pin); break;
    }
}

static int output_map1 (unsigned channel)
{
    switch (channel) {
        case 2: return 1;   // 0001 = U3TX
    }

    syslog(LOG_ERR, "uart%u: cannot map TX pin, group 1", channel);
    return 0;
}

static int output_map2 (unsigned channel)
{
    switch (channel) {
        case 0: return 1;   // 0001 = U1TX
        case 4: return 3;   // 0011 = U5TX
    }
    syslog(LOG_ERR, "uart%u: cannot map TX pin, group 2", channel);
    return 0;
}

static int output_map3 (unsigned channel)
{
    switch (channel) {
    case 3: return 2;   // 0010 = U4TX
    case 5: return 4;   // 0100 = U6TX
    }
    syslog(LOG_ERR, "uart%u: cannot map TX pin, group 3", channel);
    return 0;
}

static int output_map4 (unsigned channel)
{
    switch (channel) {
    case 1: return 2;   // 0010 = U2TX
    case 5: return 4;   // 0100 = U6TX
    }
    syslog(LOG_ERR, "uart%u: cannot map TX pin, group 4", channel);
    return 0;
}

/*
 * Assign UxTX signal to specified pin.
 */
static void assign_tx(int channel, int pin)
{
    switch (pin) {
    case RP('A',14): RPA14R = output_map1(channel); return;
    case RP('A',15): RPA15R = output_map2(channel); return;
    case RP('B',0):  RPB0R  = output_map3(channel); return;
    case RP('B',10): RPB10R = output_map1(channel); return;
    case RP('B',14): RPB14R = output_map4(channel); return;
    case RP('B',15): RPB15R = output_map3(channel); return;
    case RP('B',1):  RPB1R  = output_map2(channel); return;
    case RP('B',2):  RPB2R  = output_map4(channel); return;
    case RP('B',3):  RPB3R  = output_map2(channel); return;
    case RP('B',5):  RPB5R  = output_map1(channel); return;
    case RP('B',6):  RPB6R  = output_map4(channel); return;
    case RP('B',7):  RPB7R  = output_map3(channel); return;
    case RP('B',8):  RPB8R  = output_map3(channel); return;
    case RP('B',9):  RPB9R  = output_map1(channel); return;
    case RP('C',13): RPC13R = output_map2(channel); return;
    case RP('C',14): RPC14R = output_map1(channel); return;
    case RP('C',1):  RPC1R  = output_map1(channel); return;
    case RP('C',2):  RPC2R  = output_map4(channel); return;
    case RP('C',3):  RPC3R  = output_map3(channel); return;
    case RP('C',4):  RPC4R  = output_map2(channel); return;
    case RP('D',0):  RPD0R  = output_map4(channel); return;
    case RP('D',10): RPD10R = output_map1(channel); return;
    case RP('D',11): RPD11R = output_map2(channel); return;
    case RP('D',12): RPD12R = output_map3(channel); return;
    case RP('D',14): RPD14R = output_map1(channel); return;
    case RP('D',15): RPD15R = output_map2(channel); return;
    case RP('D',1):  RPD1R  = output_map4(channel); return;
    case RP('D',2):  RPD2R  = output_map1(channel); return;
    case RP('D',3):  RPD3R  = output_map2(channel); return;
    case RP('D',4):  RPD4R  = output_map3(channel); return;
    case RP('D',5):  RPD5R  = output_map4(channel); return;
    case RP('D',6):  RPD6R  = output_map1(channel); return;
    case RP('D',7):  RPD7R  = output_map2(channel); return;
    case RP('D',9):  RPD9R  = output_map3(channel); return;
    case RP('E',3):  RPE3R  = output_map3(channel); return;
    case RP('E',5):  RPE5R  = output_map2(channel); return;
    case RP('E',8):  RPE8R  = output_map4(channel); return;
    case RP('E',9):  RPE9R  = output_map3(channel); return;
    case RP('F',0):  RPF0R  = output_map2(channel); return;
    case RP('F',12): RPF12R = output_map3(channel); return;
    case RP('F',13): RPF13R = output_map4(channel); return;
    case RP('F',1):  RPF1R  = output_map1(channel); return;
    case RP('F',2):  RPF2R  = output_map4(channel); return;
    case RP('F',3):  RPF3R  = output_map4(channel); return;
    case RP('F',4):  RPF4R  = output_map1(channel); return;
    case RP('F',5):  RPF5R  = output_map2(channel); return;
    case RP('F',8):  RPF8R  = output_map3(channel); return;
    case RP('G',0):  RPG0R  = output_map2(channel); return;
    case RP('G',1):  RPG1R  = output_map1(channel); return;
    case RP('G',6):  RPG6R  = output_map3(channel); return;
    case RP('G',7):  RPG7R  = output_map2(channel); return;
    case RP('G',8):  RPG8R  = output_map1(channel); return;
    case RP('G',9):  RPG9R  = output_map4(channel); return;
    }
//    log ("uart%u: cannot map TX pin %c%d\n",
//        channel, &pin_name[pin>>4], pin & 15);
}

// Inits UART
// UART is configured by setting baud rate, 8N1
// Interrupts are not enabled in this function
void uart_init(u8_t unit, u32_t brg, u32_t mode, u32_t qs) {
    register struct uart_reg *reg;
    int rx = 0, tx = 0;

    unit--;

    // If UART is init test for current queue size. If new queue size is greater
    // delete current queue for creating a new one
    if (uart[unit].init) {
        if (qs > uart[unit].qs) {
            vQueueDelete(uart[unit].q);
            uart[unit].q  = xQueueCreate(qs, sizeof(u8_t));
        }
    } else {
        if (qs) {
            uart[unit].q  = xQueueCreate(qs, sizeof(u8_t));
        } else {
            uart[unit].q = NULL;
        }
    }
    
    uart[unit].qs = qs; 
    
    // Enable module
    switch (unit) {
        case 0: PMD5CLR = U1MD; break;
        case 1: PMD5CLR = U2MD; break;
        case 2: PMD5CLR = U3MD; break;
        case 3: PMD5CLR = U4MD; break;
        case 4: PMD5CLR = U5MD; break;
        case 5: PMD5CLR = U6MD; break;
    }
    
    reg = uart[unit].regs;
    
    reg->brg = ((double)PBCLK2_HZ / (double)(brg * 16)) - 1;
    reg->sta = 0;
    reg->mode = mode | PIC32_UMODE_ON; // UART Enable

    // Assign rx / tx pins
    switch (unit) {
        case 0:
            rx = (UART1_PINS & 0xff00) >> 8;
            tx = UART1_PINS & 0x00ff;
            break;
        case 1:
            rx = (UART2_PINS & 0xff00) >> 8;
            tx = UART2_PINS & 0x00ff;
            break;
        case 2:
            rx = (UART3_PINS & 0xff00) >> 8;
            tx = UART3_PINS & 0x00ff;
            break;
        case 3:
            rx = (UART4_PINS & 0xff00) >> 8;
            tx = UART4_PINS & 0x00ff;
            break;
        case 4:
            rx = (UART5_PINS & 0xff00) >> 8;
            tx = UART5_PINS & 0x00ff;
            break;
        case 5:
            rx = (UART6_PINS & 0xff00) >> 8;
            tx = UART6_PINS & 0x00ff;
            break;
    }

    assign_rx(unit, rx);
    assign_tx(unit, tx);
    
    reg->staset = PIC32_USTA_URXEN |       // Receiver Enable
  	          PIC32_USTA_UTXEN |	   // Transmit Enable
                  PIC32_USTA_URXISEL_NEMP;

    if (!uart[unit].console) {
        syslog(LOG_INFO, "%s: at pins rx=%c%d/tx=%c%d",uart[unit].name,
                gpio_portname(rx), gpio_pinno(rx),
                gpio_portname(tx), gpio_pinno(tx));

        syslog(LOG_INFO, "%s: speed %d bauds",uart[unit].name,brg);
    }

    uart[unit].init = 1;
}

// Enable UART interrupts
void uart_init_interrupts(u8_t unit) {
    register struct uart_irq *uirq;

    unit--;

    if (uart[unit].irq_init) {
        return;
    }

    uirq = &uart[unit].irqs;

    *uirq->clear_er_ipc = uirq->ipc_er_mask;
    *uirq->clear_rx_ipc = uirq->ipc_rx_mask;
  //  *uirq->clear_tx_ipc = uirq->ipc_tx_mask;

    *uirq->enable_er_ipc = uirq->ipc_er_mask & 0x08080808;
    *uirq->enable_rx_ipc = uirq->ipc_rx_mask & 0x08080808;
//    *uirq->enable_tx_ipc = uirq->ipc_tx_mask & 0x08080808;

    *uirq->enable_er_intr = uirq->er_mask;
    *uirq->enable_rx_intr = uirq->rx_mask;
    //*uirq->enable_tx_intr = uirq->tx_mask;

    uart[unit].irq_init = 1;

    if (!uart[unit].console) {
		syslog(LOG_INFO, "%s: interrupts %d/%d/%d",uart[unit].name,
              uart[unit].irqs.rx,uart[unit].irqs.tx,uart[unit].irqs.er);
	}
}

// Gets the UART queue
QueueHandle_t *uart_get_queue(u8_t unit) {
    unit--;

    return  uart[unit].q;
 }

// Writes a byte to the UART
void uart_write(u8_t unit, char byte) {
    register struct uart_reg *reg;

    unit--;
    reg = uart[unit].regs;

    while (reg->sta & PIC32_USTA_UTXBF);
    reg->txreg = byte;
    
    if (uart[unit].debug) {
        uart_write(CONSOLE_UART, byte);
    }
}

// Writes a null-terminated string to the UART
void uart_writes(u8_t unit, char *s) {
    register struct uart_reg *reg;

    unit--;
    reg = uart[unit].regs;

    while (*s) {
        while (reg->sta & PIC32_USTA_UTXBF);
        reg->txreg = *s;

        if (uart[unit].debug) {
            uart_write(CONSOLE_UART, *s);
        }

        s++;
   }
}

// Reads a byte from uart
u8_t uart_read(u8_t unit, char *c, uint32_t timeout) {
   // Get UART register
    unit--;
	
    if (timeout != portMAX_DELAY) {
        timeout = portTICK_PERIOD_MS * timeout;
    }
	
    if (xQueueReceive(uart[unit].q, c, (TickType_t)(timeout)) == pdTRUE) {
        if (uart[unit].debug) {
            uart_write(CONSOLE_UART, *c);
        }
        
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
uint8_t uart_reads(u8_t unit, char *buff, u8_t crlf, uint32_t timeout) {
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
static uint8_t _uart_wait_response(u8_t unit, char *command, uint8_t echo, char *ret, uint8_t substring, uint32_t timeout, int nargs, va_list pargs) {
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
uint8_t uart_wait_response(u8_t unit, char *command, uint8_t echo, char *ret, uint8_t substring, uint32_t timeout, int nargs, ...) {
    va_list pargs;

    va_start(pargs, nargs);

    uint8_t ok = _uart_wait_response(unit, command, echo, ret, substring, timeout, nargs, pargs);

    va_end(pargs);

    return ok;
}

// Sends a command to a device connected to the UART and waits for a response
uint8_t uart_send_command(u8_t unit, char *command, uint8_t echo, uint8_t crlf, char *ret, uint8_t substring, uint32_t timeout, int nargs, ...) {
    uint8_t ok = 0;

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
char *uart_name(u8_t unit) {
    return uart[unit - 1].name;
}

// Interupt handler
void uart_intr_rx(u8_t unit) {
    register struct uart_reg *reg;
    register struct uart_irq *uirq;
    BaseType_t xHigherPriorityTaskWoken;
    u8_t byte;
    int queue;
    int signal;

    xHigherPriorityTaskWoken = pdFALSE;

    reg = uart[unit].regs;
    uirq = &uart[unit].irqs;
	
    // Receive
    while (reg->sta & PIC32_USTA_URXDA) {
        queue = 1;
		
        // Read received byte
        byte = reg->rxreg;
                      
        // Signal handling
        if (unit == CONSOLE_UART - 1) {
            if (byte == 0x04) {
                if (!status_get(STATUS_LUA_RUNNING)) {
                    uart_writes(CONSOLE_UART, "Lua RTOS-booting\r\n");
                } else {
                    uart_writes(CONSOLE_UART, "Lua RTOS-running\r\n");
                }

                status_set(STATUS_LUA_ABORT_BOOT_SCRIPTS);
                
                queue = 0;
            } else if (byte == 0x03) {
                signal = SIGINT;
                if (_pthread_has_signal(signal)) {
                	_pthread_queue_signal(signal);
                    queue = 0;
                }
            }
        }
        
        if (queue) {
            // Put byte to UART queue
            xQueueSendFromISR(uart[unit].q, &byte, &xHigherPriorityTaskWoken);    
		}
    }

    // Clear rx
    *uirq->clear_rx_intr = uirq->rx_mask;
	
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void uart_intr_tx(u8_t unit) {
    register struct uart_irq *uirq;

    uirq = &uart[unit].irqs;

    // Clear rx interrupts
    *uirq->clear_tx_intr = uirq->tx_mask;
}

void uart_intr_er(u8_t unit) {
    register struct uart_reg *reg;
    register struct uart_irq *uirq;
    
    reg = uart[unit].regs;
    uirq = &uart[unit].irqs;

    // Error
    if (reg->sta & PIC32_USTA_OERR) {
        reg->staclr = PIC32_USTA_OERR;
    }

    // Clear er interrupts
    *uirq->clear_er_intr = uirq->er_mask;
}

void uart_pins(int unit, unsigned char *rx, unsigned char *tx) {
    int channel = unit - 1;

    switch (channel) {
        case 0:
            *rx = UART1_PINS >> 8 & 0xFF;
            *tx = UART1_PINS & 0xFF;
            break;
        case 1:
            *rx = UART2_PINS >> 8 & 0xFF;
            *tx = UART2_PINS & 0xFF;
            break;
        case 2:
            *rx = UART3_PINS >> 8 & 0xFF;
            *tx = UART3_PINS & 0xFF;
            break;
        case 3:
            *rx = UART4_PINS >> 8 & 0xFF;
            *tx = UART4_PINS & 0xFF;
            break;
        case 4:
            *rx = UART5_PINS >> 8 & 0xFF;
            *tx = UART5_PINS & 0xFF;
            break;
        case 5:
            *rx = UART6_PINS >> 8 & 0xFF;
            *tx = UART6_PINS & 0xFF;
            break;
    }
}

void uart_debug(int unit, int debug) {
    uart[unit - 1].debug = debug;   
}

int uart_get_br(int unit) {
    register struct uart_reg *reg;
    int divisor;
    unit--;

    reg = uart[unit].regs;
    divisor = reg->brg;

    return ((double)PBCLK2_HZ / (double)(16 * (divisor + 1)));
}

int uart_inited(int unit) {
    unit--;
    return (uart[unit].init && uart[unit].irq_init);
}

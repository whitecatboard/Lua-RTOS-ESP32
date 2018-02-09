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

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "freertos/xtensa_api.h"

#include "esp_types.h"
#include "esp_err.h"
#include "esp_intr.h"
#include "esp_attr.h"
#include "soc/soc.h"
#include "soc/uart_reg.h"
#include "soc/io_mux_reg.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>

#include <sys/macros.h>
#include <sys/status.h>
#include <sys/driver.h>
#include <sys/syslog.h>
#include <sys/delay.h>
#include <sys/_signal.h>

#include <pthread.h>
#include <drivers/uart.h>
#include <drivers/gpio.h>
#include <drivers/cpu.h>

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
// Driver locks
static driver_unit_lock_t uart_locks[NUART];
#endif

// Register drivers and errors
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
DRIVER_REGISTER_BEGIN(UART,uart,uart_locks,NULL,uart_lock_resources);
#else
DRIVER_REGISTER_BEGIN(UART,uart,NULL,NULL,uart_lock_resources);
#endif
	DRIVER_REGISTER_ERROR(UART, uart, CannotSetup, "can't setup", UART_ERR_CANT_INIT);
	DRIVER_REGISTER_ERROR(UART, uart, InvalidUnit, "invalid unit", UART_ERR_INVALID_UNIT);
	DRIVER_REGISTER_ERROR(UART, uart, InvalidDataBits, "invalid data bits", UART_ERR_INVALID_DATA_BITS);
	DRIVER_REGISTER_ERROR(UART, uart, InvalidParity, "invalid parity", UART_ERR_INVALID_PARITY);
	DRIVER_REGISTER_ERROR(UART, uart, InvalidStopBits, "invalid stop bits", UART_ERR_INVALID_STOP_BITS);
	DRIVER_REGISTER_ERROR(UART, uart, NotEnoughtMemory, "not enough memory", UART_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(UART, uart, NotSetup, "is not setup", UART_ERR_IS_NOT_SETUP);
	DRIVER_REGISTER_ERROR(UART, uart, PinNowAllowed, "pin not allowed", UART_ERR_PIN_NOT_ALLOWED);
	DRIVER_REGISTER_ERROR(UART, uart, CannotChangePinMap, "cannot change pin map once the UART unit has an attached device", UART_ERR_CANNOT_CHANGE_PINMAP);
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
DRIVER_REGISTER_END(UART,uart,uart_locks,NULL,uart_lock_resources);
#else
DRIVER_REGISTER_END(UART,uart,NULL,NULL,uart_lock_resources);
#endif

// Flags for determine some UART states
#define UART_FLAG_INIT		(1 << 0)
#define UART_FLAG_IRQ_INIT	(1 << 1)

#define ETS_UART_INUM  5
#define UART_INTR_SOURCE(u) ((u==0)?ETS_UART0_INTR_SOURCE:( (u==1)?ETS_UART1_INTR_SOURCE:((u==2)?ETS_UART2_INTR_SOURCE:0)))

// UART names
static const char *names[] = {
	"uart0",
	"uart1",
	"uart2",
};

// UART array
struct uart uart[NUART] = {
    {
        0, NULL, 0, 115200, PTHREAD_MUTEX_INITIALIZER, CONFIG_LUA_RTOS_UART0_RX,CONFIG_LUA_RTOS_UART0_TX,
    },
    {
        0, NULL, 0, 115200, PTHREAD_MUTEX_INITIALIZER, CONFIG_LUA_RTOS_UART1_RX,CONFIG_LUA_RTOS_UART1_TX,
    },
    {
        0, NULL, 0, 115200, PTHREAD_MUTEX_INITIALIZER, CONFIG_LUA_RTOS_UART2_RX,CONFIG_LUA_RTOS_UART2_TX,
    },
};

/*
 * This is for process deferred process for CONSOLE interrupt handler
 */
static xQueueHandle deferred_q = NULL;

static uint8_t console_raw = 0;

typedef struct {
	uint8_t type;
	uint8_t data;
} uart_deferred_data;

static void uart_deferred_intr_handler(void *args) {
	uart_deferred_data data;

	for(;;) {
		xQueueReceive(deferred_q, &data, portMAX_DELAY);
		if (data.type == 0) {
			_signal_queue(1, data.data);
		} else {
			if (data.data == 1) {
		    	uart_ll_lock(CONSOLE_UART);
		        uart_writes(CONSOLE_UART, "Lua RTOS-booting-ESP32\r\n");
		    	uart_ll_unlock(CONSOLE_UART);
			} else if (data.data == 2) {
		    	uart_ll_lock(CONSOLE_UART);
		        uart_writes(CONSOLE_UART, "Lua RTOS-running-ESP32\r\n");
		    	uart_ll_unlock(CONSOLE_UART);
			}
		}
	}
}

/*
 * Helper functions
 */

// Configure the UART comm parameters
static void uart_comm_param_config(int8_t unit, UartBautRate brg, UartBitsNum4Char data, UartParityMode parity, UartStopBitsNum stop) {
	wait_tx_empty(unit);

	uart_set_baudrate(unit, brg);

    WRITE_PERI_REG(UART_CONF0_REG(unit),
                   ((parity == NONE_BITS) ? 0x0 : (UART_PARITY_EN | parity))
                   | (stop << UART_STOP_BIT_NUM_S)
                   | (data << UART_BIT_NUM_S
                   | UART_TICK_REF_ALWAYS_ON_M));
}

// Configure the UART pins
static void uart_pin_config(int8_t unit, uint8_t flags) {
	wait_tx_empty(unit);

	int tx_sig, rx_sig;

    switch(unit) {
        case UART_NUM_0:
            tx_sig = U0TXD_OUT_IDX;
            rx_sig = U0RXD_IN_IDX;
            break;
        case UART_NUM_1:
            tx_sig = U1TXD_OUT_IDX;
            rx_sig = U1RXD_IN_IDX;
            break;
        case UART_NUM_2:
            tx_sig = U2TXD_OUT_IDX;
            rx_sig = U2RXD_IN_IDX;
            break;
        case UART_NUM_MAX:
            default:
            tx_sig = U0TXD_OUT_IDX;
            rx_sig = U0RXD_IN_IDX;
            break;
    }

    // Configure TX
    if ((flags & UART_FLAG_WRITE) && (uart[unit].tx >= 0)) {
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[uart[unit].tx], PIN_FUNC_GPIO);
        gpio_set_level(uart[unit].tx, 1);
        gpio_matrix_out(uart[unit].tx, tx_sig, 0, 0);
    }

    // Configure RX
    if ((flags & UART_FLAG_READ) && (uart[unit].rx >= 0)) {
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[uart[unit].rx], PIN_FUNC_GPIO);
        gpio_set_pull_mode(uart[unit].rx, GPIO_PULLUP_ONLY);
        gpio_set_direction(uart[unit].rx, GPIO_MODE_INPUT);
        gpio_matrix_in(uart[unit].rx, rx_sig, 0);
    }
}

// Determine if byte must be queued
static int IRAM_ATTR queue_byte(int8_t unit, uint8_t byte, uint8_t *status, int *signal) {
	*signal = 0;
	*status = 0;

    if (unit == CONSOLE_UART) {
        if ((byte == 0x04) && (!console_raw)) {
            if (!status_get(STATUS_LUA_RUNNING)) {
            	*status = 1;
            } else {
            	*status = 2;
            }

			status_set(STATUS_LUA_ABORT_BOOT_SCRIPTS);

            return 0;
        } else if ((byte == 0x03) && (!console_raw)) {
        	if (status_get(STATUS_LUA_RUNNING)) {
				*signal = SIGINT;
				if (_pthread_has_signal(1, *signal)) {
					return 0;
				}

				return 1;
        	} else {
        		return 0;
        	}
        }
    }
	
	if (status_get(STATUS_LUA_RUNNING) || console_raw) {
		return 1;
	} else {
		return 0;
	}
}

/*
 * Operation functions
 */

void IRAM_ATTR uart_ll_lock(int unit) {
	pthread_mutex_lock(&uart[unit].mtx);
}

void IRAM_ATTR uart_ll_unlock(int unit) {
	pthread_mutex_unlock(&uart[unit].mtx);
}

driver_error_t *uart_lock(int unit) {
	// Sanity checks
	if ((unit > CPU_LAST_UART) || (unit < CPU_FIRST_UART)) {
		return driver_error(UART_DRIVER, UART_ERR_INVALID_UNIT, NULL);
	}

	if (!((uart[unit].flags & UART_FLAG_INIT) && (uart[unit].flags & UART_FLAG_IRQ_INIT))) {
		return driver_error(UART_DRIVER, UART_ERR_IS_NOT_SETUP, NULL);
	}

	uart_ll_lock(unit);

	return NULL;
}

driver_error_t *uart_unlock(int unit) {
	// Sanity checks
	if ((unit > CPU_LAST_UART) || (unit < CPU_FIRST_UART)) {
		return driver_error(UART_DRIVER, UART_ERR_INVALID_UNIT, NULL);
	}

	if (!((uart[unit].flags & UART_FLAG_INIT) && (uart[unit].flags & UART_FLAG_IRQ_INIT))) {
		return driver_error(UART_DRIVER, UART_ERR_IS_NOT_SETUP, NULL);
	}

	uart_ll_unlock(unit);

	return NULL;
}

void IRAM_ATTR uart_ll_set_raw(uint8_t raw) {
	console_raw = raw;
}

void IRAM_ATTR uart_rx_intr_handler(void *args) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t uart_intr_status = 0;
    uart_deferred_data data;

	uint8_t byte, status;
	int signal = 0;
	int unit = (int)args;

	uart_intr_status = READ_PERI_REG(UART_INT_ST_REG(unit));

	while (uart_intr_status != 0x0) {
		if (UART_FRM_ERR_INT_ST == (uart_intr_status & UART_FRM_ERR_INT_ST)) {
			WRITE_PERI_REG(UART_INT_CLR_REG(unit), UART_FRM_ERR_INT_CLR);
		} else if (UART_RXFIFO_FULL_INT_ST == (uart_intr_status & UART_RXFIFO_FULL_INT_ST)) {
			WRITE_PERI_REG(UART_INT_CLR_REG(unit), UART_RXFIFO_FULL_INT_CLR);

			while ((READ_PERI_REG(UART_STATUS_REG(unit)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT) {
				byte = READ_PERI_REG(UART_FIFO_REG(unit)) & 0xFF;
				if (queue_byte(unit, byte, &status, &signal)) {
					// Put byte to UART queue
					xQueueSendFromISR(uart[unit].q, &byte, &xHigherPriorityTaskWoken);
				} else {
					if (signal) {
						data.type = 0;
						data.data = signal;

						xQueueSendFromISR(deferred_q, &data, &xHigherPriorityTaskWoken);
					}

					if (status) {
						data.type = 1;
						data.data = status;

						xQueueSendFromISR(deferred_q, &data, &xHigherPriorityTaskWoken);
					}
				}
			}
		} else if (UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST)) {
			WRITE_PERI_REG(UART_INT_CLR_REG(unit), UART_RXFIFO_TOUT_INT_CLR);

			while ((READ_PERI_REG(UART_STATUS_REG(unit)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT) {
				byte = READ_PERI_REG(UART_FIFO_REG(unit)) & 0xFF;
				if (queue_byte(unit, byte, &status, &signal)) {
					// Put byte to UART queue
					xQueueSendFromISR(uart[unit].q, &byte, &xHigherPriorityTaskWoken);
				} else {
					if (signal) {
						data.type = 0;
						data.data = signal;

						xQueueSendFromISR(deferred_q, &data, &xHigherPriorityTaskWoken);
					}

					if (status) {
						data.type = 1;
						data.data = status;

						xQueueSendFromISR(deferred_q, &data, &xHigherPriorityTaskWoken);
					}
				}
			}
		}

		uart_intr_status = READ_PERI_REG(UART_INT_ST_REG(unit));
	}

	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
// Lock resources needed by the UART
driver_error_t *uart_lock_resources(int unit, uint8_t flags, void *resources) {
    driver_unit_lock_error_t *lock_error = NULL;

    // Lock this pins
    if ((flags & UART_FLAG_READ) && (uart[unit].rx >= 0)) {
        if ((lock_error = driver_lock(UART_DRIVER, unit, GPIO_DRIVER, uart[unit].rx, flags, "RX"))) {
        	// Revoked lock on pin
        	return driver_lock_error(UART_DRIVER, lock_error);
        }
    }

    if ((flags & UART_FLAG_WRITE) && (uart[unit].tx >= 0)) {
        if ((lock_error = driver_lock(UART_DRIVER, unit, GPIO_DRIVER, uart[unit].tx, flags, "TX"))) {
        	// Revoked lock on pin
        	return driver_lock_error(UART_DRIVER, lock_error);
        }
    }

    return NULL;
}
#endif

driver_error_t *uart_pin_map(int unit, int rx, int tx) {
    // Sanity checks
	if ((unit > CPU_LAST_UART) || (unit < CPU_FIRST_UART)) {
		return driver_error(UART_DRIVER, UART_ERR_INVALID_UNIT, NULL);
	}

    if (uart[unit].flags & UART_FLAG_INIT) {
		return driver_error(SPI_DRIVER, UART_ERR_CANNOT_CHANGE_PINMAP, NULL);
    }

    if ((!(GPIO_ALL_IN & (GPIO_BIT_MASK << rx))) && (rx >= 0)) {
		return driver_error(UART_DRIVER, UART_ERR_PIN_NOT_ALLOWED, "rx, selected pin cannot be input");
    }

    if ((!(GPIO_ALL_OUT & (GPIO_BIT_MASK << tx))) && (tx >= 0)) {
		return driver_error(UART_DRIVER, UART_ERR_PIN_NOT_ALLOWED, "tx, selected pin cannot be input");
    }

    if (!TEST_UNIQUE2(rx, tx)) {
		return driver_error(UART_DRIVER, UART_ERR_PIN_NOT_ALLOWED, "rx, and tx must be different");
    }

    // Update rx
    if (rx >= 0) {
    	uart[unit].rx  = rx;
    }

    // Update tx
    if (tx >= 0) {
    	uart[unit].tx  = tx;
    }

	return NULL;
}

// Init UART. Interrupts are not enabled.
driver_error_t *uart_init(int8_t unit, uint32_t brg, uint8_t databits, uint8_t parity, uint8_t stop_bits, uint8_t flags, uint32_t qs) {
	// Sanity checks
	if ((unit > CPU_LAST_UART) || (unit < CPU_FIRST_UART)) {
		return driver_error(UART_DRIVER, UART_ERR_INVALID_UNIT, NULL);
	}

    if ((!(GPIO_ALL_IN & (GPIO_BIT_MASK << uart[unit].rx))) && (uart[unit].rx >= 0)) {
		return driver_error(UART_DRIVER, UART_ERR_PIN_NOT_ALLOWED, "rx, selected pin cannot be input");
    }

    if ((!(GPIO_ALL_OUT & (GPIO_BIT_MASK << uart[unit].tx))) && (uart[unit].tx >= 0)) {
		return driver_error(UART_DRIVER, UART_ERR_PIN_NOT_ALLOWED, "tx, selected pin cannot be input");
    }

    if (!TEST_UNIQUE2(uart[unit].rx, uart[unit].tx)) {
		return driver_error(UART_DRIVER, UART_ERR_PIN_NOT_ALLOWED, "rx, and tx must be different");
    }

    // Get data bits, and sanity checks
    UartBitsNum4Char esp_databits = EIGHT_BITS;
    switch (databits) {
    	case 5: esp_databits = FIVE_BITS ; break;
    	case 6: esp_databits = SIX_BITS  ; break;
    	case 7: esp_databits = SEVEN_BITS; break;
    	case 8: esp_databits = EIGHT_BITS; break;
    	default:
    		return driver_error(UART_DRIVER, UART_ERR_INVALID_DATA_BITS, NULL);
    }

    // Get parity, and sanity checks
    UartParityMode esp_parity = NONE_BITS;
    switch (parity) {
    	case 0: esp_parity = NONE_BITS;break;
    	case 1: esp_parity = EVEN_BITS;break;
    	case 2: esp_parity = ODD_BITS ;break;
    	default:
    		return driver_error(UART_DRIVER, UART_ERR_INVALID_PARITY, NULL);
    }

    // Get stop bits, and sanity checks
    UartStopBitsNum esp_stop_bits = ONE_STOP_BIT;
    switch (stop_bits) {
    	case 0: esp_stop_bits = ONE_HALF_STOP_BIT; break;
    	case 1: esp_stop_bits = ONE_STOP_BIT; break;
    	case 2: esp_stop_bits = TWO_STOP_BIT; break;
    	default:
    		return driver_error(UART_DRIVER, UART_ERR_INVALID_STOP_BITS, NULL);
    }

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    // Lock resources
    driver_error_t *error;

    if ((error = uart_lock_resources(unit, flags, NULL))) {
		return error;
	}
#endif

	// There are not errors, continue with init ...

    // Enable module
    switch (unit) {
    	case 0: periph_module_enable(PERIPH_UART0_MODULE); break;
    	case 1: periph_module_enable(PERIPH_UART1_MODULE); break;
    	case 2: periph_module_enable(PERIPH_UART2_MODULE); break;
    }

    // If the requested queue size is greater than current queue size,
	// delete it and create a new one
    if (qs > uart[unit].qs) {
		if (uart[unit].q) {
			vQueueDelete(uart[unit].q);
		}

		uart[unit].q  = xQueueCreate(qs, sizeof(uint8_t));
		if (!uart[unit].q) {
			driver_error(UART_DRIVER, UART_ERR_NOT_ENOUGH_MEMORY, NULL);
		}
	}

    // Init mutex, if needed
    if (uart[unit].mtx == PTHREAD_MUTEX_INITIALIZER) {
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&uart[unit].mtx, &attr);
    }

	// For the console, create the queue signal, and start a task for process signals
    // received from the console
	#if CONFIG_LUA_RTOS_USE_CONSOLE
		if (unit == CONSOLE_UART) {
			if (!deferred_q) {
				deferred_q = xQueueCreate(1, sizeof(uart_deferred_data));
				xTaskCreatePinnedToCore(uart_deferred_intr_handler, "uart", configMINIMAL_STACK_SIZE, NULL, 21, NULL, 0);
			}
		}
	#endif

	uart_pin_config(unit, flags);
	uart_comm_param_config(unit, brg, esp_databits, esp_parity, esp_stop_bits);

    uart[unit].brg = brg; 
    uart[unit].qs  = qs; 

    uart[unit].flags |= UART_FLAG_INIT;

    if ((flags & (UART_FLAG_READ | UART_FLAG_WRITE)) == (UART_FLAG_READ | UART_FLAG_WRITE)) {
	    syslog(LOG_INFO, "%s: at pins rx=%s%d/tx=%s%d",names[unit],
	            gpio_portname(uart[unit].rx), gpio_name(uart[unit].rx),
	            gpio_portname(uart[unit].tx), gpio_name(uart[unit].tx));
	} else if ((flags & (UART_FLAG_READ | UART_FLAG_WRITE)) == UART_FLAG_WRITE) {
	    syslog(LOG_INFO, "%s: at pins tx=%s%d",names[unit],
	            gpio_portname(uart[unit].tx), gpio_name(uart[unit].tx));
	} else if ((flags & (UART_FLAG_READ | UART_FLAG_WRITE)) == UART_FLAG_READ) {
	    syslog(LOG_INFO, "%s: at pins rx=%s%d",names[unit],
	            gpio_portname(uart[unit].rx), gpio_name(uart[unit].rx));
	}

    syslog(LOG_INFO, "%s: speed %d bauds", names[unit],brg);

    return NULL;
}

// Enable UART interrupts
driver_error_t *uart_setup_interrupts(int8_t unit) {
	if ((unit > CPU_LAST_UART) || (unit < CPU_FIRST_UART)) {
		return driver_error(UART_DRIVER, UART_ERR_INVALID_UNIT, NULL);
	}

	if (uart[unit].flags & UART_FLAG_IRQ_INIT) {
        return NULL;
    }

    uint32_t reg_val = 0;
	uint32_t mask = UART_RXFIFO_TOUT_INT_ENA_M | UART_FRM_ERR_INT_ENA_M | UART_RXFIFO_FULL_INT_ENA_M;

	esp_intr_alloc(UART_INTR_SOURCE(unit), ESP_INTR_FLAG_IRAM, uart_rx_intr_handler, (void *)((uint32_t)unit), NULL);

	WRITE_PERI_REG(UART_INT_CLR_REG(unit), 0x1ff);

	// Update CONF1 register
    reg_val = READ_PERI_REG(UART_CONF1_REG(unit)) & ~((UART_RX_FLOW_THRHD << UART_RX_FLOW_THRHD_S) | UART_RX_FLOW_EN) ;

    reg_val |= ((mask & UART_RXFIFO_TOUT_INT_ENA_M) ?
                (((2 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S) | UART_RX_TOUT_EN) : 0);

    reg_val |= ((mask & UART_FRM_ERR_INT_ENA_M) ?
                ((10 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) : 0);

    reg_val |= ((mask & UART_RXFIFO_FULL_INT_ENA_M) ?
                ((20 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S) : 0);

    WRITE_PERI_REG(UART_CONF1_REG(unit), reg_val);

    // Update INT_ENA register
    WRITE_PERI_REG(UART_INT_ENA_REG(unit), mask);
	
	syslog(LOG_INFO, "%s: interrupts enabled",names[unit]);

    uart[unit].flags |= UART_FLAG_IRQ_INIT;

	return NULL;
}

// Writes a byte to the UART
void IRAM_ATTR uart_write(int8_t unit, char byte) {
    while (((READ_PERI_REG(UART_STATUS_REG(unit)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S)) >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) >= 126);
    WRITE_PERI_REG(UART_FIFO_REG(unit), byte);
}

// Writes a null-terminated string to the UART
void IRAM_ATTR uart_writes(int8_t unit, char *s) {
    while (*s) {
	    while (((READ_PERI_REG(UART_STATUS_REG(unit)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S)) >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) >= 126);
	    WRITE_PERI_REG(UART_FIFO_REG(unit) , *s++);
   }
}

// Reads a byte from uart
uint8_t IRAM_ATTR uart_read(int8_t unit, char *c, uint32_t timeout) {
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
driver_error_t *uart_consume(int8_t unit) {
    char tmp;

	// Sanity checks
	if ((unit > CPU_LAST_UART) || (unit < CPU_FIRST_UART)) {
		return driver_error(UART_DRIVER, UART_ERR_INVALID_UNIT, NULL);
	}

	if (!((uart[unit].flags & UART_FLAG_INIT) && (uart[unit].flags & UART_FLAG_IRQ_INIT))) {
		return driver_error(UART_DRIVER, UART_ERR_IS_NOT_SETUP, NULL);
	}

    while(uart_read(unit,&tmp,1));

	return NULL;
} 

// Reads a string from the UART, ended by the CR + LF character
uint8_t uart_reads(int8_t unit, char *buff, uint8_t crlf, uint32_t timeout) {
    char c;
    int n = 0;

    for (;;) {
        if (uart_read(unit, &c, timeout)) {
            if (c == '\0') {
            	*buff = 0;
                return 1;
            } else if (c == '\n') {
            	n++;
                *buff = 0;
                return 1;
            } else {
                if ((c == '\r') && !crlf) {
                	n++;
                    *buff = 0;
                    return 1;
                } else {
                    if (c != '\r') {
                    	n++;
                        *buff++ = c;
                    }
                }
            }
        } else {
        	*buff = 0;
            return (n > 0);
        }
    }

    return 0;
}

// Read from the UART and waits for a response
static uint8_t _uart_wait_response(int8_t unit, char *command, uint8_t echo, char *ret, uint8_t substring, uint32_t timeout, int nargs, va_list pargs) {
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
uint8_t uart_wait_response(int8_t unit, char *command, uint8_t echo, char *ret, uint8_t substring, uint32_t timeout, int nargs, ...) {
    va_list pargs;

    va_start(pargs, nargs);

    uint8_t ok = _uart_wait_response(unit, command, echo, ret, substring, timeout, nargs, pargs);

    va_end(pargs);

    return ok;
}

// Sends a command to a device connected to the UART and waits for a response
uint8_t uart_send_command(int8_t unit, char *command, uint8_t echo, uint8_t crlf, char *ret, uint8_t substring, uint32_t timeout, int nargs, ...) {
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
const char *uart_name(int8_t unit) {
    return names[unit - 1];
}

// Gets the UART queue
QueueHandle_t *uart_get_queue(int8_t unit) {
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

int uart_is_setup(int unit) {
    return ((uart[unit].flags & UART_FLAG_INIT) && (uart[unit].flags & UART_FLAG_IRQ_INIT));
}

void uart_stop(int unit) {
	int cunit = 0;

	for(cunit = 0;cunit < NUART; cunit++) {
		if ((unit == -1) || (cunit == unit)) {
		    WRITE_PERI_REG(UART_CONF0_REG(unit), 0);
		}
	}
}

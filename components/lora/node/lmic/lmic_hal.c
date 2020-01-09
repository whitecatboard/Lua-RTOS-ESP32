/*
 * Lua RTOS, LMIC hardware abstraction layer
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276 || CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272

#include "lmic.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_attr.h"
#include "esp_intr.h"
#include "soc/gpio_reg.h"
#include "soc/rtc_cntl_reg.h"

#include "driver/gpio.h"

#include "lora.h"

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/delay.h>
#include <sys/syslog.h>
#include <sys/mutex.h>
#include <sys/driver.h>
#include <sys/status.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <drivers/power_bus.h>

#define LMIC_HAL_INTERRUPTS 0

/*
 * Mutex for protect critical regions
 */
static struct mtx lmic_hal_mtx;

// Start time
static struct timeval start_tv;

static int spi_device;

#if LMIC_HAL_INTERRUPTS
static int nestedInterrupts  = 0;
#endif

 /*
  * This is an event group handler for sleep / resume os_runloop.
  * When os_runloop has nothing to do waits for an event.
  */
 #define evLMIC_SLEEP ( 1 << 0 )

// This queue is for resume the os_runloop loop
xQueueHandle lmicSleepEvent;

// This queue is for send a command to LMIC for ensure that
// this command is executed into the LMIC thread
xQueueHandle lmicCommand;

/*
 * This is the LMIC interrupt handler. This interrupt is attached to the transceiver
 * DIO lines. LMIC uses only DIO0, DIO1 and DIO2 lines.
 *
 * When an interrupt is triggered we queue it's processing, setting an LMIC callback, which
 * will be executed in the next iteration of the os_runloop routine.
 *
 */

 static void IRAM_ATTR dio_intr_handler(void* arg) {
	uint32_t d = 1;

	xQueueSendFromISR(lmicSleepEvent, &d, NULL);
}

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
driver_error_t *lmic_lock_resources(int unit, void *resources) {
	driver_error_t *error;
    driver_unit_lock_error_t *lock_error = NULL;

	if ((error = spi_lock_bus_resources(CONFIG_LUA_RTOS_LORA_SPI, DRIVER_ALL_FLAGS))) {
		return error;
	}

	#if ((CONFIG_LUA_RTOS_POWER_BUS_PIN == -1) && (CONFIG_LUA_RTOS_LORA_RST >= 0))
    if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LORA_RST, DRIVER_ALL_FLAGS, "RST"))) {
    	// Revoked lock on pin
    	return driver_lock_error(LORA_DRIVER, lock_error);
    }
	#endif

	#if CONFIG_LUA_RTOS_LORA_DIO0 >= 0
	if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LORA_DIO0, DRIVER_ALL_FLAGS, "DIO0"))) {
		// Revoked lock on pin
		return driver_lock_error(LORA_DRIVER, lock_error);
	}
	#endif

	#if CONFIG_LUA_RTOS_LORA_DIO1 >= 0
	if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LORA_DIO1, DRIVER_ALL_FLAGS, "DIO1"))) {
		// Revoked lock on pin
		return driver_lock_error(LORA_DRIVER, lock_error);
	}
	#endif

	#if CONFIG_LUA_RTOS_LORA_DIO2 >= 0
	if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LORA_DIO2, DRIVER_ALL_FLAGS, "DIO2"))) {
		// Revoked lock on pin
		return driver_lock_error(LORA_DRIVER, lock_error);
	}
	#endif

	return NULL;
}
#endif

driver_error_t *hal_init (void) {
	driver_error_t *error;

	// Init SPI bus
	if ((error = spi_setup(CONFIG_LUA_RTOS_LORA_SPI, 1, CONFIG_LUA_RTOS_LORA_CS, 0, LMIC_SPI_KHZ * 1000, SPI_FLAG_WRITE | SPI_FLAG_READ, &spi_device))) {
        syslog(LOG_ERR, "lmic cannot open spi%u", CONFIG_LUA_RTOS_LORA_SPI);
        return error;
    }

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	// Lock pins
    if ((error = lmic_lock_resources(0, NULL))) {
    	return error;
    }
#endif

	syslog(LOG_INFO, "lmic is at spi%d, pin cs=%s%d", CONFIG_LUA_RTOS_LORA_SPI,
        gpio_portname(CONFIG_LUA_RTOS_LORA_CS), gpio_name(CONFIG_LUA_RTOS_LORA_CS)
	);

	#if ((CONFIG_LUA_RTOS_POWER_BUS_PIN == -1) && (CONFIG_LUA_RTOS_LORA_RST >= 0))
		// Init RESET pin
		gpio_pin_output(CONFIG_LUA_RTOS_LORA_RST);
	#endif

	if (!status_get(STATUS_ISR_SERVICE_INSTALLED)) {
		gpio_install_isr_service(0);

		status_set(STATUS_ISR_SERVICE_INSTALLED, 0x00000000);
	}

	// Init DIO pins
	#if CONFIG_LUA_RTOS_LORA_DIO0 >= 0
		if ((error = gpio_isr_attach(CONFIG_LUA_RTOS_LORA_DIO0, dio_intr_handler, GPIO_INTR_POSEDGE, (void*)0))) {
			error->msg = "DIO0";
			return error;
		}
	#endif

	#if CONFIG_LUA_RTOS_LORA_DIO1 >= 0
		if ((error = gpio_isr_attach(CONFIG_LUA_RTOS_LORA_DIO1, dio_intr_handler, GPIO_INTR_POSEDGE, (void*)1))) {
			error->msg = "DIO1";
			return error;
		}
	#endif

	#if CONFIG_LUA_RTOS_LORA_DIO2 >= 0
		if ((error = gpio_isr_attach(CONFIG_LUA_RTOS_LORA_DIO2, dio_intr_handler, GPIO_INTR_POSEDGE, (void*)2))) {
			error->msg = "DIO2";
			return error;
		}
	#endif

	lmicSleepEvent = xQueueCreate(1, sizeof(uint32_t));
	lmicCommand = xQueueCreate(1, sizeof(lmic_command_t));

	// Create mutex
    mtx_init(&lmic_hal_mtx, NULL, NULL, 0);

    // Get start time
	gettimeofday(&start_tv, NULL);

	return NULL;
}

/*
 * drive radio NSS pin (0=low, 1=high).
 */
void IRAM_ATTR hal_pin_nss (u1_t val) {
    if (!val) {
		spi_ll_select(spi_device);
	} else {
		spi_ll_deselect(spi_device);
	}
}

/*
 * drive radio RX/TX pins (0=rx, 1=tx).
 */
void hal_pin_rxtx (u1_t val) {
	
}

/*
 * control radio RST pin (0=low, 1=high, 2=floating)
 */
void hal_pin_rst (u1_t val) {
	#if (CONFIG_LUA_RTOS_LORA_CONNECTED_TO_POWER_BUS && (CONFIG_LUA_RTOS_POWER_BUS_PIN >= 0))
		if (val == 1) {
			pwbus_off();
			delay(1);
			pwbus_on();
		} else if (val == 0) {
			pwbus_off();
			delay(1);
			pwbus_on();
		} else {
			delay(5);
		}
	#else
		#if CONFIG_LUA_RTOS_LORA_RST >= 0
			if (val == 1) {
				gpio_pin_output(CONFIG_LUA_RTOS_LORA_RST);
				gpio_pin_set(CONFIG_LUA_RTOS_LORA_RST);
			} else if (val == 0) {
				gpio_pin_output(CONFIG_LUA_RTOS_LORA_RST);
				gpio_pin_clr(CONFIG_LUA_RTOS_LORA_RST);
			} else {
				gpio_pin_input(CONFIG_LUA_RTOS_LORA_RST);
			}
		#endif
	#endif
}

/*
 * perform 8-bit SPI transaction with radio.
 *   - write given byte 'outval'
 *   - read byte and return value
 */
u1_t IRAM_ATTR hal_spi (u1_t outval) {
	u1_t readed;

	spi_ll_transfer(spi_device, outval, &readed);

	return readed;
}

void IRAM_ATTR hal_disableIRQs (void) {
#if LMIC_HAL_INTERRUPTS
	mtx_lock(&lmic_hal_mtx);
	if (nestedInterrupts++ == 0) {
		mtx_unlock(&lmic_hal_mtx);
		portDISABLE_INTERRUPTS();
	} else {
		mtx_unlock(&lmic_hal_mtx);
	}
#endif
}

void IRAM_ATTR hal_enableIRQs (void) {
#if LMIC_HAL_INTERRUPTS
	mtx_lock(&lmic_hal_mtx);
	if (--nestedInterrupts == 0) {
		mtx_unlock(&lmic_hal_mtx);
		portENABLE_INTERRUPTS();
	} else {
		mtx_unlock(&lmic_hal_mtx);
	}
#endif
}

void IRAM_ATTR hal_resume (void) {
	uint32_t d = 0;

	xQueueSend(lmicSleepEvent, &d, 0);
}

void hal_sleep (void) {
	uint32_t d = 0;

	xQueueReceive(lmicSleepEvent, &d, portMAX_DELAY);
	if (d == 1) {
		radio_irq_handler(0);
	}
}

/*
 * In ESP32 RTC runs at 150.000 hz.
 *
 * Each RTC tick has a period of (100/15) usecs. If we factorize this value we have that
 * (100 / 15) usecs = ((2 * 5 * 2 * 4) / (3 * 5)) usecs = (20 / 3) usecs.
 *
 * LMIC needs a tick period between 15.5 usecs and 100 usecs, so we have to multiply RTC ticks
 * periods for give LMIC ticks periods. This causes, for example, that if we multiply RTC ticks
 * periods by 3 we have an exact period time of 20 usecs (20 / 3) usecs * 3 = 20 usecs.
 *
 * For that reason Lua RTOS is configured to count 1 LMIC tick every 3 RTC ticks, so, for LMIC:
 *
 * US_PER_OSTICK = 20
 * OSTICKS_PER_SEC = 50000
 *
 */
u8_t IRAM_ATTR hal_ticks () {
	struct timeval tv;
	u8_t ticks;

	gettimeofday(&tv, NULL);

	ticks  = (tv.tv_sec - start_tv.tv_sec) * (1000000 / US_PER_OSTICK);
	ticks += (tv.tv_usec - start_tv.tv_usec) / US_PER_OSTICK;

	return ticks;
}

/*
 * Return 1 if target time is closed.
 */
static u1_t is_close(u8_t target) {
	u1_t res = (hal_ticks() >= target);

	return res;
}

/*
 * busy-wait until specified timestamp (in ticks) is reached.
 */
void hal_waitUntil (u8_t time) {
    while (!is_close(time)) {
    	udelay(US_PER_OSTICK);
    }
}

/*
 * check and rewind timer for target time.
 *   - return 1 if target time is close
 *   - otherwise rewind timer for target time or full period and return 0
 */
u1_t hal_checkTimer (u8_t targettime) {
	return (is_close(targettime));
}

/*
 * perform fatal failure action.
 *   - called by assertions
 *   - action could be HALT or reboot
 */
void hal_failed (char *file, int line) {
	syslog(LOG_ERR, "%lu: assert at %s, line %s\n", (u4_t)os_getTime(), file, line);

	for(;;);
}

void hal_lmic_tx(int port, uint8_t *payload, uint8_t payload_len, uint8_t cnf) {
	lmic_command_t command;

	command.command = LMICTx;
	command.tx.port = port;
	command.tx.payload = payload;
	command.tx.payload_len = payload_len;
	command.tx.cnf = cnf;

	xQueueSend(lmicCommand, &command, portMAX_DELAY);

	hal_resume();
}

void hal_lmic_join() {
	lmic_command_t command;

	command.command = LMICJoin;

	xQueueSend(lmicCommand, &command, portMAX_DELAY);

	hal_resume();
}

void hal_lmic_command() {
	lmic_command_t command;

	if (xQueueReceive(lmicCommand, &command, 0)) {
		if (command.command == LMICTx) {
			LMIC_setTxData2(command.tx.port, command.tx.payload,  command.tx.payload_len,  command.tx.cnf);

			free(command.tx.payload);
		} else if (command.command == LMICJoin) {
			LMIC_startJoining();
		}
	}
}

#endif

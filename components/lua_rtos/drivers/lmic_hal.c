/*
 * Lua RTOS, LMIC hardware abstraction layer
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

#if LUA_USE_LORA
#if CONFIG_LUA_RTOS_USE_LMIC

#include "lmic.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_attr.h"
#include "soc/gpio_reg.h"
#include "soc/rtc_cntl_reg.h"

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/delay.h>
#include <sys/syslog.h>
#include <sys/mutex.h>
#include <sys/driver.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <drivers/lora.h>
#include <drivers/power_bus.h>

extern unsigned port_interruptNesting[portNUM_PROCESSORS];

#define DIO_MASK ((1ULL << CONFIG_LUA_RTOS_LMIC_DIO0) | (1ULL << CONFIG_LUA_RTOS_LMIC_DIO1) | (1ULL << CONFIG_LUA_RTOS_LMIC_DIO2))
#define DIO_MASK_L (DIO_MASK & 0xffffffff)
#define DIO_MASK_H (DIO_MASK >> 32)

/*
 * Mutex for protect critical regions
 */
static struct mtx lmic_hal_mtx;

/*
 * This variables are for doing things only one time.
 *
 * nested is for enable / disable interrupts once.
 * sleeped is for sleep os_runloop once.
 * resumed is for resume os_runloop once.
 *
 * For example, enable / disable interrupts are done as follows:
 *
 *   enable - disable - enable - disable
 *
 */
static int nested  = 0;
static int sleeped = 0;
static int resumed = 0;

// DIO lines interrupt handler
static gpio_isr_handle_t dio_handle;

 /*
  * This is an event group handler for sleep / resume os_runloop.
  * When os_runloop has nothing to do waits for an event.
  */
 #define evLMIC_SLEEP ( 1 << 0 )

 EventGroupHandle_t lmicSleepEvent;

/*
 * This is the LMIC interrupt handler. This interrupt is attached to the transceiver
 * DIO lines. LMIC uses only DIO0, DIO1 and DIO2 lines.
 *
 * When an interrupt is triggered we queue it's processing, setting an LMIC callback, which
 * will be executed in the next iteration of the os_runloop routine.
 *
 */
static void IRAM_ATTR dio_intr_handler(void *args) {
	u4_t status_l = READ_PERI_REG(GPIO_STATUS_REG) & GPIO_STATUS_INT & DIO_MASK_L;
	u4_t status_h = READ_PERI_REG(GPIO_STATUS1_REG) & GPIO_STATUS1_INT & DIO_MASK_H;

	/*
	 * Don't change the position of this code. Interrupt status must be clean
	 * in this point.
	 */
	WRITE_PERI_REG(GPIO_STATUS_W1TC_REG, status_l & DIO_MASK_L);
	WRITE_PERI_REG(GPIO_STATUS1_W1TC_REG, status_h & DIO_MASK_H);

	if (status_l | status_h) {
		radio_irq_handler(0);
		hal_resume();
	}
}

driver_error_t *lmic_lock_resources(int unit, void *resources) {
    driver_unit_lock_error_t *lock_error = NULL;

	#if !CONFIG_LUA_RTOS_USE_POWER_BUS
    if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LMIC_RST))) {
    	// Revoked lock on pin
    	return driver_lock_error(LORA_DRIVER, lock_error);
    }
	#endif

	#if CONFIG_LUA_RTOS_LMIC_DIO0
	if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LMIC_DIO0))) {
			// Revoked lock on pin
			return driver_lock_error(LORA_DRIVER, lock_error);
	}
	#endif

	#if CONFIG_LUA_RTOS_LMIC_DIO1
	if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LMIC_DIO1))) {
		// Revoked lock on pin
		return driver_lock_error(LORA_DRIVER, lock_error);
	}
	#endif

	#if CONFIG_LUA_RTOS_LMIC_DIO2
	if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LMIC_DIO2))) {
		// Revoked lock on pin
		return driver_lock_error(LORA_DRIVER, lock_error);
	}
	#endif

	return NULL;
}

driver_error_t *hal_init (void) {
	driver_error_t *error;

	// Init SPI bus
    if ((error = spi_init(CONFIG_LUA_RTOS_LMIC_SPI, 1))) {
        syslog(LOG_ERR, "lmic cannot open spi%u", CONFIG_LUA_RTOS_LMIC_SPI);
        return error;
    }
    
    // Lock pins
    if ((error = lmic_lock_resources(0, NULL))) {
    	return error;
    }

    spi_set_cspin(CONFIG_LUA_RTOS_LMIC_SPI, CONFIG_LUA_RTOS_LMIC_CS);
    spi_set_speed(CONFIG_LUA_RTOS_LMIC_SPI, LMIC_SPI_KHZ);

    if (spi_cs_gpio(CONFIG_LUA_RTOS_LMIC_SPI) >= 0) {
        syslog(LOG_INFO, "lmic is at %s, cs=%s%d/dio0=%s%d/dio1=%s%d/dio2=%s%d",
			spi_name(CONFIG_LUA_RTOS_LMIC_SPI),
			gpio_portname(spi_cs_gpio(CONFIG_LUA_RTOS_LMIC_SPI)), spi_cs_gpio(CONFIG_LUA_RTOS_LMIC_SPI),
			gpio_portname(CONFIG_LUA_RTOS_LMIC_DIO0), CONFIG_LUA_RTOS_LMIC_DIO0,
			gpio_portname(CONFIG_LUA_RTOS_LMIC_DIO1), CONFIG_LUA_RTOS_LMIC_DIO1,
			gpio_portname(CONFIG_LUA_RTOS_LMIC_DIO2), CONFIG_LUA_RTOS_LMIC_DIO2
		);
    }
	
	#if !CONFIG_LUA_RTOS_USE_POWER_BUS
		// Init RESET pin
		gpio_pin_output(CONFIG_LUA_RTOS_LMIC_RST);
	#endif

	gpio_isr_register(&dio_intr_handler, NULL, 0, &dio_handle);

	// Init DIO pins
	#if CONFIG_LUA_RTOS_LMIC_DIO0
		gpio_pin_input(CONFIG_LUA_RTOS_LMIC_DIO0);
		gpio_set_intr_type(CONFIG_LUA_RTOS_LMIC_DIO0, GPIO_INTR_POSEDGE);
	#endif
	
	#if CONFIG_LUA_RTOS_LMIC_DIO1
		gpio_pin_input(CONFIG_LUA_RTOS_LMIC_DIO1);
		gpio_set_intr_type(CONFIG_LUA_RTOS_LMIC_DIO1, GPIO_INTR_POSEDGE);
	#endif

	#if CONFIG_LUA_RTOS_LMIC_DIO2
		gpio_pin_input(CONFIG_LUA_RTOS_LMIC_DIO2);
		gpio_set_intr_type(CONFIG_LUA_RTOS_LMIC_DIO2, GPIO_INTR_POSEDGE);
	#endif

	// Create lmicSleepEvent
	lmicSleepEvent = xEventGroupCreate();

	// Create mutex
    mtx_init(&lmic_hal_mtx, NULL, NULL, 0);

	// Enable DIO interrupts
	#if CONFIG_LUA_RTOS_LMIC_DIO0
		gpio_intr_enable(CONFIG_LUA_RTOS_LMIC_DIO0);
	#endif

	#if CONFIG_LUA_RTOS_LMIC_DIO1
		gpio_intr_enable(CONFIG_LUA_RTOS_LMIC_DIO1);
	#endif

	#if CONFIG_LUA_RTOS_LMIC_DIO2
		gpio_intr_enable(CONFIG_LUA_RTOS_LMIC_DIO2);
	#endif

    return NULL;
}

/*
 * drive radio NSS pin (0=low, 1=high).
 */
void IRAM_ATTR hal_pin_nss (u1_t val) {
    spi_set_cspin(CONFIG_LUA_RTOS_LMIC_SPI, CONFIG_LUA_RTOS_LMIC_CS);

    if (!val) {
		spi_select(CONFIG_LUA_RTOS_LMIC_SPI);
	} else {
		spi_deselect(CONFIG_LUA_RTOS_LMIC_SPI);
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
	#if CONFIG_LUA_RTOS_USE_POWER_BUS
		if (val == 1) {
			esp_intr_disable((intr_handle_t)dio_handle);
			pwbus_off();
			delay(1);
			pwbus_on();
			esp_intr_enable((intr_handle_t)dio_handle);
		} else if (val == 0) {
			esp_intr_disable((intr_handle_t)dio_handle);
			pwbus_off();
			delay(1);
			pwbus_on();
			esp_intr_enable((intr_handle_t)dio_handle);
		} else {
			delay(5);
		}
	#else
		if (val == 1) {
			gpio_pin_output(CONFIG_LUA_RTOS_LMIC_RST);
			gpio_pin_set(CONFIG_LUA_RTOS_LMIC_RST);
		} else if (val == 0) {
			gpio_pin_output(CONFIG_LUA_RTOS_LMIC_RST);
			gpio_pin_clr(CONFIG_LUA_RTOS_LMIC_RST);
		} else {
			gpio_pin_input(CONFIG_LUA_RTOS_LMIC_RST);
		}
	#endif
}

/*
 * perform 8-bit SPI transaction with radio.
 *   - write given byte 'outval'
 *   - read byte and return value
 */
u1_t IRAM_ATTR hal_spi (u1_t outval) {
	u1_t readed;

	spi_transfer(CONFIG_LUA_RTOS_LMIC_SPI, outval, &readed);

	return readed;
}

void IRAM_ATTR hal_disableIRQs (void) {
	int disable = 0;

	mtx_lock(&lmic_hal_mtx);

	if (nested++ == 0) {
		disable = 1;
	}

	mtx_unlock(&lmic_hal_mtx);

	if (disable) {
		portDISABLE_INTERRUPTS();
	}
}

void IRAM_ATTR hal_enableIRQs (void) {
	int enable = 0;

	mtx_lock(&lmic_hal_mtx);

	if (--nested == 0) {
		enable = 1;
	}

	mtx_unlock(&lmic_hal_mtx);

	if (enable) {
		portENABLE_INTERRUPTS();
	}
}

void IRAM_ATTR hal_resume (void) {
	int resume = 0;

	mtx_lock(&lmic_hal_mtx);

	if (resumed == 0) {
		resumed = 1;
		resume  = 1;
	}

	mtx_unlock(&lmic_hal_mtx);

	if (resume) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		if (port_interruptNesting[xPortGetCoreID()] != 0) {
			xEventGroupSetBitsFromISR(lmicSleepEvent, evLMIC_SLEEP, &xHigherPriorityTaskWoken);
		} else {
			xEventGroupSetBits(lmicSleepEvent, evLMIC_SLEEP);
		}
	}
}

void hal_sleep (void) {
	int sleep = 0;

	mtx_lock(&lmic_hal_mtx);

	if (sleeped == 0) {
		sleeped = 1;
		sleep   = 1;
	}

	mtx_unlock(&lmic_hal_mtx);

	if (sleep) {
		xEventGroupWaitBits(lmicSleepEvent, evLMIC_SLEEP, pdTRUE, pdFALSE, portMAX_DELAY);

		mtx_lock(&lmic_hal_mtx);
		sleeped = 0;
		resumed = 0;
		mtx_unlock(&lmic_hal_mtx);
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
	u8_t microseconds;

	gettimeofday(&tv, NULL);

	microseconds  = tv.tv_sec * 1000000;
	microseconds += tv.tv_usec;

	return (microseconds / US_PER_OSTICK);
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
    	udelay(1);
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

#endif
#endif

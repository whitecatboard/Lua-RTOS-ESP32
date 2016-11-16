/*
 * Lua RTOS, LMIC hardware abstraction layer
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

#include "luartos.h"

#if LUA_USE_LORA
#if USE_LMIC

#include "lmic.h"

#include "freertos/FreeRtos.h"
#include "freertos/timers.h"

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/delay.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>

#include "soc/gpio_reg.h"

/*
 * This is an adapter function for call radio_irq_handler from a callback
 */
static osjob_t dio_job;

static void deferred_dio_intr_handler(osjob_t *j) {
	radio_irq_handler(0);
}

/*
 * This is the LMIC interrupt handler. This interrupt is attached to the transceiver
 * DIO lines. LMIC uses only DIO0, DIO1 and DIO2 lines.
 *
 * When an interrupt is triggered we queue it's processing, setting an LMIC callback, which
 * will be executed in the next iteration of the os_runloop routine.
 *
 */
static void dio_intr_handler(void *args) {
	u4_t status_l = READ_PERI_REG(GPIO_STATUS_REG) & GPIO_STATUS_INT;
	u4_t status_h = READ_PERI_REG(GPIO_STATUS1_REG) & GPIO_STATUS1_INT;

	WRITE_PERI_REG(GPIO_STATUS_W1TC_REG, status_l);
	WRITE_PERI_REG(GPIO_STATUS1_W1TC_REG, status_h);

	os_setCallback(&dio_job, deferred_dio_intr_handler);
}

void hal_init (void) {
	// Init SPI bus
    if (spi_init(LMIC_SPI) != 0) {
        syslog(LOG_ERR, "lmic cannot open spi%u", LMIC_SPI);
        return;
    }
    
    spi_set_cspin(LMIC_SPI, LMIC_CS);
    spi_set_speed(LMIC_SPI, LMIC_SPI_KHZ);

    if (spi_cspin(LMIC_SPI) >= 0) {
        syslog(LOG_INFO, "lmic is at %s, cs=%s%d",
        spi_name(LMIC_SPI), spi_csname(LMIC_SPI), spi_cspin(LMIC_SPI));
    }
	
	// Init RESET pin
	gpio_pin_output(LMIC_RST);

	gpio_isr_register(ETS_GPIO_INUM, &dio_intr_handler, NULL);

	// Init DIO pins
	if (LMIC_DIO0) {
		gpio_pin_input(LMIC_DIO0);
		gpio_set_intr_type(LMIC_DIO0, GPIO_INTR_POSEDGE);
		gpio_intr_enable(LMIC_DIO0);
	}
	
	if (LMIC_DIO1) {
		gpio_pin_input(LMIC_DIO1);
		gpio_set_intr_type(LMIC_DIO1, GPIO_INTR_POSEDGE);
		gpio_intr_enable(LMIC_DIO1);
	}
	
	if (LMIC_DIO2) {
		gpio_pin_input(LMIC_DIO2);
		gpio_set_intr_type(LMIC_DIO2, GPIO_INTR_POSEDGE);
		gpio_intr_enable(LMIC_DIO2);
	}
}

/*
 * drive radio NSS pin (0=low, 1=high).
 */
void hal_pin_nss (u1_t val) {
    spi_set_cspin(LMIC_SPI, LMIC_CS);

    if (!val) {
		spi_select(LMIC_SPI);
	} else {
		spi_deselect(LMIC_SPI);	
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
	if (val == 1) {
		gpio_pin_output(LMIC_RST);
		gpio_pin_set(LMIC_RST);
	} else if (val == 0) {
		gpio_pin_output(LMIC_RST);
		gpio_pin_clr(LMIC_RST);
	} else {
		gpio_pin_input(LMIC_RST);
	}	
}

/*
 * perform 8-bit SPI transaction with radio.
 *   - write given byte 'outval'
 *   - read byte and return value
 */
u1_t hal_spi (u1_t outval) {
	return spi_transfer(LMIC_SPI, outval);
}

static int nested = 0;

void hal_disableIRQs (void) {
	if (nested == 0) {
		portDISABLE_INTERRUPTS();
	}

	nested++;
}

void hal_enableIRQs (void) {
	if (--nested == 0) {
		portENABLE_INTERRUPTS();
	}
}

void hal_sleep (void) {
}

u4_t hal_ticks () {
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return ((((tv.tv_sec * 1000000 + tv.tv_usec) * 15) / 100) / 3);
}

// Returns the number of ticks until time. Negative values indicate that
// time has already passed.
static s4_t delta_time(u4_t time) {
    return (s4_t)(time - hal_ticks());
}

/*
 * busy-wait until specified timestamp (in ticks) is reached.
 */
void hal_waitUntil (u4_t time) {
    while (delta_time(time)  >= 0) {
    	udelay(1);
    }
}

/*
 * check and rewind timer for target time.
 *   - return 1 if target time is close
 *   - otherwise rewind timer for target time or full period and return 0
 */
u1_t hal_checkTimer (u4_t targettime) {
	return delta_time(targettime) <= 0;
}

/*
 * perform fatal failure action.
 *   - called by assertions
 *   - action could be HALT or reboot
 */
void hal_failed (char *file, int line) {
	printf("assert: at %s, line %d\n", file, line);
	
	for(;;);
}

#endif
#endif

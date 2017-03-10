/*
 * Lua RTOS, NEOPIXEL WS2812B driver
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

#include "freertos/FreeRTOS.h"

#include "neopixel.h"

#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/gpio.h>

#define NEO_CYCLES(n) ((double)n / (double)((double)1000000000L / (double)CPU_HZ))

static neopixel_chipset_t chipset[1] = {
	{NEO_CYCLES(350), NEO_CYCLES(900), NEO_CYCLES(900), NEO_CYCLES(350), NEO_CYCLES(50000)}, // WS2812B
};

static uint8_t neopixel_gpio;

driver_error_t *neopixel_setup(neopixel_controller_t controller, uint8_t gpio, uint8_t *unit) {
	neopixel_gpio = gpio;

	gpio_pin_output(neopixel_gpio);

	*unit = controller;

	return NULL;
}

driver_error_t *neopixel_send(uint8_t unit, uint32_t *rgbs, uint32_t len) {
	int bit, i;
	uint32_t *rgb;
	uint32_t pulseH;
	uint32_t pulseL;

	uint32_t c, s;

	portDISABLE_INTERRUPTS();

	c = xthal_get_ccount();
	rgb = rgbs;
	for(i=0;i < len;i++) {
		for(bit=23;bit >= 0; bit--) {
			s = c;

			pulseH = ((*rgb) & (1 << bit))?chipset[unit].t1h:chipset[unit].t0h;
			pulseL = ((*rgb) & (1 << bit))?chipset[unit].t1l:chipset[unit].t0l;

			gpio_ll_pin_set(neopixel_gpio);
			while (((c = xthal_get_ccount()) - s) < pulseH);

			gpio_ll_pin_clr(neopixel_gpio);
			while (((c = xthal_get_ccount()) - s) < pulseL);
		}

		rgb++;
	}

	portENABLE_INTERRUPTS();

	return NULL;
}

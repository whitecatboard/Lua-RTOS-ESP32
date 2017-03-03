/*
 * Lua RTOS, clock driver
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

#include "freertos/FreeRTOS.h"
#include "esp_freertos_hooks.h"
#include "esp_attr.h"

#include <sys/time.h>

#include <drivers/gpio.h>

#if USE_LED_ACT
static volatile uint64_t tdelta = 0;

unsigned int activity = 0;

void IRAM_ATTR newTick(void) {
    tdelta++;
    if (tdelta == configTICK_RATE_HZ) {
        // 1 second since last second
        tdelta = 0;

        if (activity <= 0) {
			activity = 0;
			gpio_ll_pin_inv(LED_ACT);
		}
    }
}

#endif

void _clock_init(void) {
	#if USE_LED_ACT
    esp_register_freertos_tick_hook(&newTick);
	#endif
}

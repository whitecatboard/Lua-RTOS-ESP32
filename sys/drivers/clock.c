/*
 * Lua RTOS, clock driver
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

#include "FreeRTOS.h"
#include "build.h"

#include <sys/time.h>
#include <sys/drivers/gpio.h>

#define ATTR

#if PLATFORM_ESP32
#include "esp_attr.h"

#undef ATTR
#define ATTR IRAM_ATTR
#endif

#if PLATFORM_ESP8266
#undef ATTR
#define ATTR IRAM
#endif

static volatile clock_t  tticks = 0;     // Number of ticks
static volatile clock_t  tdelta = 0;   	 // Delta ticks from last second
static volatile uint32_t tseconds = 0;   // Seconds counter
static volatile uint32_t tuseconds = 0;  // Microseconds counter

#if LED_ACT
unsigned int activity = 0;
#endif

void _clock_init(void) {
    tseconds = BUILD_TIME;
}

void ATTR __newTick(void) {
    // Increment internal tick counter
    tticks++;

    // Increment usecs
    tuseconds = tuseconds + configTICK_RATE_HZ;

    // Increment delta ticks
    tdelta++;
    if (tdelta == configTICK_RATE_HZ) {
        // 1 second since last second
        tdelta = 0;
        tuseconds = 0;

        tseconds++;
        
		#if LED_ACT
        	if (activity <= 0) {
            	activity = 0;
            	gpio_pin_inv(LED_ACT);
        	}
		#endif
    }
}

clock_t ATTR __ticks() {
    return tticks;
}

uint32_t ATTR __tseconds() {
	return tseconds;
}

uint32_t ATTR __tuseconds() {
	return tuseconds;
}
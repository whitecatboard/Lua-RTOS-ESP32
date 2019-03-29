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
 * Lua RTOS, power bus driver
 *
 */

#include "luartos.h"

#if (CONFIG_LUA_RTOS_POWER_BUS_PIN >= 0)

#include <time.h>

#include <sys/delay.h>
#include <sys/driver.h>
#include <sys/mutex.h>

#include <drivers/gpio.h>
#include <drivers/power_bus.h>

static struct mtx mtx;

// How many devices are connected to the power bus? When no devices are connected the power bus can
// turn off.
static int power = 0;

// When was started the powe bus
static struct timespec uptime;

// Register driver and errors
static void _pwbus_init();

DRIVER_REGISTER_BEGIN(PWBUS,pwbus,0,_pwbus_init,NULL);
	DRIVER_REGISTER_ERROR(PWBUS, pwbus, InvalidPin, "invalid pin", PWBUS_ERR_INVALID_PIN);
DRIVER_REGISTER_END(PWBUS,pwbus,0,_pwbus_init,NULL);

/*
 * Helper functions
 */
static void _pwbus_init() {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	driver_lock(PWBUS_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_POWER_BUS_PIN, DRIVER_ALL_FLAGS, NULL);
#endif

	mtx_init(&mtx, NULL, NULL, 0);

	gpio_pin_output(CONFIG_LUA_RTOS_POWER_BUS_PIN);
	gpio_pin_clr(CONFIG_LUA_RTOS_POWER_BUS_PIN);
	power = 0;
}

/*
 * Operation functions
 */
driver_error_t *pwbus_on() {
    mtx_lock(&mtx);

	if (power > 0) {
		power++;

	    mtx_unlock(&mtx);
	    return NULL;
	}

	power++;

	clock_gettime(CLOCK_MONOTONIC, &uptime);

	gpio_pin_set(CONFIG_LUA_RTOS_POWER_BUS_PIN);

	// Wait some time for power stabilization
	delay(CONFIG_LUA_RTOS_POWER_BUS_DELAY);

    mtx_unlock(&mtx);

	return NULL;
}

driver_error_t *pwbus_off() {
    mtx_lock(&mtx);

    if (power == 1) {
    	uptime.tv_sec = 0;
    	uptime.tv_nsec = 0;

		gpio_pin_clr(CONFIG_LUA_RTOS_POWER_BUS_PIN);
		mtx_unlock(&mtx);
    }

    if (power > 0) {
    	power--;
    }

    mtx_unlock(&mtx);

    return NULL;
}

uint64_t pwbus_uptime() {
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);

	return ((now.tv_sec * 1000 + (now.tv_nsec / 1000000)) - (uptime.tv_sec * 1000 + (uptime.tv_nsec / 1000000)));
}

#endif

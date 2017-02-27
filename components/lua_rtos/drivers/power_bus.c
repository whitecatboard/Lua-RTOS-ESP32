/*
 * Lua RTOS, power bus driver
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

#if CONFIG_LUA_RTOS_USE_POWER_BUS

#include <sys/driver.h>

#include <drivers/gpio.h>
#include <drivers/power_bus.h>

static int power = 0;

DRIVER_REGISTER_ERROR(PWBUS, pwbus, InvalidPin, "invalid pin", PWBUS_ERR_INVALID_PIN);

/*
 * Helper functions
 */
static void _pwbus_init() {
	gpio_pin_output(CONFIG_LUA_RTOS_POWER_BUS_PIN);
	gpio_pin_clr(CONFIG_LUA_RTOS_POWER_BUS_PIN);
	power = 0;
}

/*
 * Operation functions
 */
driver_error_t *pwbus_on() {
	if (power) return NULL;

	gpio_pin_set(CONFIG_LUA_RTOS_POWER_BUS_PIN);
	power = 1;

	return NULL;
}

driver_error_t *pwbus_off() {
	if (!power) return NULL;

	gpio_pin_clr(CONFIG_LUA_RTOS_POWER_BUS_PIN);
	power = 0;

	return NULL;
}

DRIVER_REGISTER(PWBUS,pwbus,NULL,_pwbus_init,NULL);

#endif

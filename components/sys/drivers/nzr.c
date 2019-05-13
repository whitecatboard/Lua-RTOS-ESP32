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
 * Lua RTOS, NZR driver
 *
 */

#include "freertos/FreeRTOS.h"
#include "nzr.h"

#include <string.h>

#include <sys/driver.h>
#include <sys/delay.h>
#include <sys/list.h>

#include <drivers/rmt.h>
#include <drivers/gpio.h>

// Register driver and messages
void nzr_init();

DRIVER_REGISTER_BEGIN(NZR,nzr,0,nzr_init,NULL);
    DRIVER_REGISTER_ERROR(NZR, nzr, NotEnoughtMemory, "not enough memory", NZR_ERR_NOT_ENOUGH_MEMORY);
    DRIVER_REGISTER_ERROR(NZR, nzr, InvalidUnit, "invalid unit", NRZ_ERR_INVALID_UNIT);
DRIVER_REGISTER_END(NZR,nzr,0,nzr_init,NULL);

// List of units
struct list nzr_list;

/*
 * Operation functions
 */
void nzr_init() {
    // Init  list
    lstinit(&nzr_list, 0, LIST_DEFAULT);
}

driver_error_t *nzr_setup(nzr_timing_t *timing, uint8_t gpio, uint32_t *unit) {
    driver_error_t *error;
    nzr_instance_t *instance;

    // Allocate space for instance
    instance = (nzr_instance_t *)calloc(1, sizeof(nzr_instance_t));
    if (!instance) {
        return driver_error(NZR_DRIVER, NZR_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    // Copy values to instance
    memcpy(&instance->timings, timing, sizeof(nzr_timing_t));
    instance->gpio = gpio;

    // Add instance
    if (lstadd(&nzr_list, instance, (int *)unit)) {
        free(instance);

        return driver_error(NZR_DRIVER, NZR_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

#if CONFIG_LUA_RTOS_LUA_USE_RMT
    // The preferred implementation uses the RMT to avoid disabling interrupts.
    // If there a not RMT channels available we use the bit bang implementation.
    int rmt_device;

    error = rmt_setup_tx(gpio, RMTPulseRangeNSEC, RMTIdleL, NULL, &rmt_device);
    if (!error) {
    	// Use RMT
		instance->deviceid = rmt_device;
	} else {
#endif
		// Not possible
		free(error);

		// Use bit bang implementation.
		instance->deviceid = 0xffffffff;

	#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
		driver_unit_lock_error_t *lock_error = NULL;

		// Lock the GPIO
		if ((lock_error = driver_lock(NZR_DRIVER, *unit, GPIO_DRIVER, gpio, DRIVER_ALL_FLAGS, NULL))) {
			lstremove(&nzr_list, *unit, 1);
			// Revoked lock on pin
			return driver_lock_error(NZR_DRIVER, lock_error);
		}
	#endif

			// Configure GPIO as output
			if ((error = gpio_pin_output(gpio))) {
				lstremove(&nzr_list, *unit, 1);

				return error;
			}

			gpio_ll_pin_clr(gpio);
#if CONFIG_LUA_RTOS_LUA_USE_RMT
	}
#endif

    return NULL;
}

driver_error_t *nzr_send(uint32_t unit, uint8_t *data, uint32_t bits) {
    nzr_instance_t *instance;
    uint8_t mask;
    uint32_t pulseH;
    uint32_t pulseL;
    uint32_t c, s, bit;

    // Get instance
    if (lstget(&nzr_list, (int)unit, (void **)&instance)) {
        return driver_error(NZR_DRIVER, NRZ_ERR_INVALID_UNIT, NULL);
    }

#if CONFIG_LUA_RTOS_LUA_USE_RMT
    if (instance->deviceid != 0xffffffff) {
        // RMT implementation
        driver_error_t *error = NULL;

        // Create buffer
        rmt_item_t *buffer = calloc(bits, sizeof(rmt_item_t));
        if (!buffer) {
            return driver_error(NZR_DRIVER, NZR_ERR_NOT_ENOUGH_MEMORY, NULL);
        }

        // Prepare data
        rmt_item_t *cbuffer = buffer;

        mask = 0x80;
        for(bit = 0;bit < bits; bit++) {
            pulseH = ((*data) & mask)?instance->timings.n.t1h:instance->timings.n.t0h;
            pulseL = ((*data) & mask)?instance->timings.n.t1l:instance->timings.n.t0l;

            cbuffer->duration0 = pulseH;
            cbuffer->level0 = 1;

            cbuffer->duration1 = pulseL;
            cbuffer->level1 = 0;

            cbuffer++;

            mask = mask >> 1;
            if (mask == 0) {
                mask = 0x80;
                data++;
            }
        }

        error = rmt_tx(instance->deviceid, buffer, bits);
        if (error) {
            free(buffer);
            return error;
        }

        free(buffer);
    } else {
#endif
        // Bit bang implementation
        portDISABLE_INTERRUPTS();

        mask = 0x80;
        c = xthal_get_ccount();
        for(bit = 0;bit < bits; bit++) {
            s = c;

            pulseH = ((*data) & mask)?instance->timings.c.t1h:instance->timings.c.t0h;
            pulseL = ((*data) & mask)?instance->timings.c.t1l:instance->timings.c.t0l;

            gpio_ll_pin_set(instance->gpio);
            while (((c = xthal_get_ccount()) - s) < pulseH);

            s = c;
            gpio_ll_pin_clr(instance->gpio);
            while (((c = xthal_get_ccount()) - s) < pulseL);

            mask = mask >> 1;
            if (mask == 0) {
                mask = 0x80;
                data++;
            }
        }

        portENABLE_INTERRUPTS();
#if CONFIG_LUA_RTOS_LUA_USE_RMT
    }
#endif

    udelay(instance->timings.res / 1000);

    return NULL;
}

driver_error_t *nzr_unsetup(uint32_t unit) {
    nzr_instance_t *instance;

    // Get instance
    if (lstget(&nzr_list, (int)unit, (void **)&instance)) {
        return driver_error(NZR_DRIVER, NRZ_ERR_INVALID_UNIT, NULL);
    }

#if CONFIG_LUA_RTOS_LUA_USE_RMT
    if (instance->deviceid != 0xffffffff) {
        // RMT implementation
        rmt_unsetup_tx(instance->deviceid);
    } else {
#endif
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    // Unlock the GPIO
    driver_unlock(NZR_DRIVER, unit, GPIO_DRIVER, instance->gpio);
#endif
#if CONFIG_LUA_RTOS_LUA_USE_RMT
    }
#endif

    return NULL;
}

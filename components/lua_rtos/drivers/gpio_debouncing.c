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
 * Lua RTOS, gpio debouncing routines
 *
 */

#include "esp_err.h"
#include "esp_attr.h"
#include "driver/timer.h"

#include <string.h>

#include <sys/status.h>
#include <sys/driver.h>
#include <drivers/gpio.h>
#include <drivers/gpio_debouncing.h>
#include <drivers/cpu.h>
#include <drivers/timer.h>

#if EXTERNAL_GPIO
#include <drivers/pca9xxx.h>
#endif

// Debouncing data
static debouncing_t *debouncing = NULL;

// Max threshold data
static uint32_t max_threshold = 0;

void IRAM_ATTR gpio_isr(void *args) {
	uint8_t pin = ((uint32_t)args);

	// Update mask
	if (pin < 40) {
		debouncing->mask |= (uint64_t)(GPIO_BIT_MASK << pin);
	}
#if EXTERNAL_GPIO
	else {
		debouncing->mask_ext |= (uint64_t)(GPIO_BIT_MASK << (pin - 40));
	}
#endif

	// Start timer
	int groupn, idx;
	get_group_idx(0, &groupn, &idx);
	if(groupn == 0) {
		TIMERG0.hw_timer[idx].config.enable = 1;
	} else {
		TIMERG1.hw_timer[idx].config.enable = 1;
	}
}

void IRAM_ATTR debouncing_isr(void *args) {
#if !EXTERNAL_GPIO
	if (debouncing->mask == 0) {
		// Stop timer
		int groupn, idx;
		get_group_idx(0, &groupn, &idx);
		if(groupn == 0) {
			TIMERG0.hw_timer[idx].config.enable = 0;
		} else {
			TIMERG1.hw_timer[idx].config.enable = 0;
		}

		return;
	}
#else
	if ((debouncing->mask == 0) && (debouncing->mask_ext == 0)) {
		// Stop timer
		int groupn, idx;
		get_group_idx(0, &groupn, &idx);
		if(groupn == 0) {
			TIMERG0.hw_timer[idx].config.enable = 0;
		} else {
			TIMERG1.hw_timer[idx].config.enable = 0;
		}

		return;
	}
#endif

	// Get all GPIO values
	uint64_t current = (((uint64_t)GPIO.in1.data << 32) | GPIO.in) & debouncing->mask;

#if EXTERNAL_GPIO
	uint64_t current_ext = pca_9xxx_pin_get_all(&current_ext) & debouncing->mask_ext;
#endif

	// Test for changes
	uint64_t mask = 1;
	uint8_t i;

	// Internal
	for(i = CPU_FIRST_GPIO;i < 40;i++) {
		if (mask & debouncing->mask) {
			if ((debouncing->latch & mask) != (current & mask)) {
				// GPIO has changed
				if (!((debouncing->time[i] <= debouncing->threshold[i]))) {
					if (debouncing->callback[i]) {
						debouncing->callback[i](debouncing->arg[i], (current & mask) != 0);
					}

					// Set new value
					if (current & mask) {
						debouncing->latch |= mask;
					} else {
						debouncing->latch &= ~mask;
					}

					// Set GPIO time to 0
					debouncing->time[i] = 0;

					// Clear mask
					debouncing->mask &= ~mask;
				} else {
					// Increment time for GPIO
					debouncing->time[i]++;
				}
			}
		}

		mask = mask << 1;
	}

	// External
	#if EXTERNAL_GPIO
	mask = 1;
	for(i= 40;i <= CPU_LAST_GPIO;i++) {
		if (mask & debouncing->mask_ext) {
			if ((debouncing->latch_ext & mask) != (current_ext & mask)) {
				// GPIO has changed
				if (!((debouncing->time[i] <= debouncing->threshold[i]))) {
					if (debouncing->callback[i]) {
						debouncing->callback[i](debouncing->arg[i], (current_ext & mask) != 0);
					}

					// Set new value
					if (current_ext & mask) {
						debouncing->latch_ext |= mask;
					} else {
						debouncing->latch_ext &= ~mask;
					}

					// Set GPIO time to 0
					debouncing->time[i] = 0;

					// Clear mask
					debouncing->mask_ext &= ~mask;
				} else {
					// Increment time for GPIO
					debouncing->time[i]++;
				}
			}
		}

		mask = mask << 1;
	}
	#endif
}

driver_error_t *gpio_debouncing_register(uint8_t pin, uint16_t threshold, gpio_debouncing_callback_t callback, void *args) {
	driver_error_t *error;
	uint8_t setup = 0;

	// Allocate space for debouncing data
	if (!debouncing) {
		portDISABLE_INTERRUPTS();
		debouncing = calloc(1, sizeof(debouncing_t));
		if (!debouncing) {
			portENABLE_INTERRUPTS();
			return driver_error(GPIO_DRIVER, GPIO_ERR_NOT_ENOUGH_MEMORY, NULL);
		}

		 mtx_init(&debouncing->mtx, NULL, NULL, 0);

		portENABLE_INTERRUPTS();
	} else {
		setup = 1;
	}

	mtx_lock(&debouncing->mtx);

	// Configure all masked gpio as input, pull-up enabled, and get initial state

	// GPIO is an input GPIO?
	if (gpio_is_input(pin)) {
		gpio_pin_input(pin);
		gpio_pin_pullup(pin);

		// Get initial state
		if (gpio_ll_pin_get(pin)) {
			if (pin < 40) {
				debouncing->latch |= (GPIO_BIT_MASK << pin);
			}
			#if EXTERNAL_GPIO
			else {
				debouncing->latch_ext |= (GPIO_BIT_MASK << (pin - 40));
			}
			#endif
		} else {
			if (pin < 40) {
				debouncing->latch &= ~(GPIO_BIT_MASK << pin);
			}
			#if EXTERNAL_GPIO
			else {
				debouncing->latch_ext &= ~(GPIO_BIT_MASK << (pin - 40));
			}
			#endif
		}

		// Store threshold value in timer period units (round-up)
		debouncing->threshold[pin] = (threshold + (GPIO_DEBOUNCING_PERIOD - 1)) / GPIO_DEBOUNCING_PERIOD;

		// Store callback
		debouncing->callback[pin] = callback;
		debouncing->arg[pin] = args;

		if (debouncing->threshold[pin] > max_threshold) {
			max_threshold = debouncing->threshold[pin];
		}

		gpio_isr_attach(pin, gpio_isr, GPIO_PIN_INTR_ANYEDGE, (void *)((uint32_t)pin));
	} else {
		mtx_unlock(&debouncing->mtx);

		return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
	}

	if (!setup) {
		// Configure a timer, but don't start timer, because timer is started when a
		// GPIO interrupt is raised
		if ((error = tmr_setup(0, GPIO_DEBOUNCING_PERIOD, debouncing_isr, 0))) {
			mtx_unlock(&debouncing->mtx);

			return error;
		}
	}

	mtx_unlock(&debouncing->mtx);

	return NULL;
}

driver_error_t *gpio_debouncing_unregister(uint8_t pin) {
	if (debouncing) {
		portDISABLE_INTERRUPTS();
		debouncing->arg[pin] = NULL;
		debouncing->callback[pin] = NULL;
		debouncing->threshold[pin] = 0;
		debouncing->time[pin] = 0;
		portENABLE_INTERRUPTS();
	}

	return NULL;
}

void gpio_debouncing_force_isr(void *args) {
	gpio_isr(args);
}

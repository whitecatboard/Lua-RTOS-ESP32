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

// Debouncing data
static debouncing_t *debouncing = NULL;

// Max threshold data
static uint32_t max_threshold = 0;

void IRAM_ATTR gpio_isr(void *args) {
	// Update mask
	debouncing->mask |= (uint64_t)(1 << ((uint32_t)args));

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
	if (!debouncing->mask) {
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

	// Get all GPIO values
	uint64_t current = (((uint64_t)GPIO.in1.data << 32) | GPIO.in) & debouncing->mask;

	// Test for changes
	uint64_t mask = 1;
	uint8_t i;

	for(i= CPU_FIRST_GPIO;i <= CPU_LAST_GPIO;i++) {
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
}

driver_error_t *gpio_debouncing_register(uint64_t pins, uint16_t threshold, gpio_debouncing_callback_t callback, void *args) {
	driver_error_t *error;
	gpio_config_t io_conf;
	uint8_t setup = 0;

	portDISABLE_INTERRUPTS();

	// Allocate space for debouncing data
	if (!debouncing) {
		debouncing = calloc(1, sizeof(debouncing_t));
		if (!debouncing) {
			portENABLE_INTERRUPTS();
			return driver_error(GPIO_DRIVER, GPIO_ERR_NOT_ENOUGH_MEMORY, NULL);
		}
	} else {
		setup = 1;
	}

	// Configure all masked gpio as input, pull-up enabled, and get initial state
	uint64_t mask = 1;
	uint8_t i;
	for(i=CPU_FIRST_GPIO;i <= CPU_LAST_GPIO;i++) {
		// GPIO is masked?
		if (mask & pins) {
			// GPIO is an input GPIO?
			if (mask & GPIO_ALL_IN) {
				io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
			    io_conf.mode = GPIO_MODE_INPUT;
			    io_conf.pin_bit_mask = mask;
			    io_conf.pull_down_en = 0;
			    io_conf.pull_up_en = 1;

			    gpio_config(&io_conf);

			    // Get initial state
				if (gpio_ll_pin_get(i)) {
					debouncing->latch |= mask;
				} else {
					debouncing->latch &= ~mask;
				}

				// Store threshold value in timer period units (round-up)
				debouncing->threshold[i] = (threshold + (GPIO_DEBOUNCING_PERIOD - 1) / GPIO_DEBOUNCING_PERIOD);

				// Store callback
				debouncing->callback[i] = callback;
				debouncing->arg[i] = args;

				if (debouncing->threshold[i] > max_threshold) {
					max_threshold = debouncing->threshold[i];
				}

				// Install GPIO isr
				// Configure interrupts
			    if (!status_get(STATUS_ISR_SERVICE_INSTALLED)) {
			    	gpio_install_isr_service(0);

			    	status_set(STATUS_ISR_SERVICE_INSTALLED);
			    }

			    gpio_isr_handler_add(i, gpio_isr, (void *)((uint32_t)i));
			} else {
				portENABLE_INTERRUPTS();
				return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
			}
		}

		mask = mask << 1;
	}

	if (!setup) {
		// Configure a timer, but don't start timer, because timer is started when a
		// GPIO interrupt is raised
		if ((error = tmr_setup(0, GPIO_DEBOUNCING_PERIOD, debouncing_isr, 0))) {
			portENABLE_INTERRUPTS();
			return error;
		}
	}

	portENABLE_INTERRUPTS();

	return NULL;
}

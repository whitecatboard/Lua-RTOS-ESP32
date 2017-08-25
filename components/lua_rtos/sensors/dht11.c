/*
 * Lua RTOS, DHT11 sensor (temperature & humidity)
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_DHT11

#include "freertos/FreeRTOS.h"
#include "esp_attr.h"

#include <time.h>

#include <sys/driver.h>
#include <sys/delay.h>
#include <sys/sleep.h>

#include <drivers/gpio.h>
#include <drivers/sensor.h>

driver_error_t *dht11_setup(sensor_instance_t *unit);
driver_error_t *dht11_acquire(sensor_instance_t *unit, sensor_value_t *values);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) dht11_sensor = {
	.id = "DHT11",
	.interface = {
		GPIO_INTERFACE,
	},
	.data = {
		{.id = "temperature", .type = SENSOR_DATA_FLOAT},
		{.id = "humidity"   , .type = SENSOR_DATA_FLOAT},
	},
	.setup = dht11_setup,
	.acquire = dht11_acquire
};

/*
 * Helper functions
 */
static int dht11_bus_monitor(uint8_t pin, uint8_t level, int16_t timeout) {
	uint8_t val = level;
	uint32_t start, end, elapsed;

	// Get start time
	start = xthal_get_ccount();

	if (timeout > 0) {
		while (val == level) {
			gpio_pin_get(pin, &val);
			end = xthal_get_ccount();
			elapsed = (int)((end - start) / (CPU_HZ / (1000000 * (CPU_HZ / CORE_TIMER_HZ))));
			if (elapsed >= timeout) {
				return -1;
			}
		}
	} else {
		while (val == level) {
			gpio_pin_get(pin, &val);
		}
	}

	end = xthal_get_ccount();
	elapsed = (int)((end - start) / (CPU_HZ / (1000000 * (CPU_HZ / CORE_TIMER_HZ))));

	return elapsed;
}

/*
 * Operation functions
 */
driver_error_t *dht11_setup(sensor_instance_t *unit) {
	// Get pin from instance
	uint8_t pin = unit->setup[0].gpio.gpio;

	// Configure pin as input(pull-up enable)
	gpio_pin_input(pin);
	gpio_pin_pullup(pin);

	return NULL;
}

driver_error_t *dht11_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	uint8_t data[5] = {0,0,0,0,0}; // dht11 returns 5 byte of date in each transfer
	uint8_t byte = 0;		 	   // Current byte transferred by sensor
	uint8_t cnt = 7;			   // Current bit into current byte transferred by sensor
	int bit = 0;                   // Current transferred bit by sensor
	int elapsed;                   // Elapsed time in usecs between level transitions 0->1 / 1 ->0

	// Get pin from instance
	uint8_t pin = unit->setup[0].gpio.gpio;

	portDISABLE_INTERRUPTS();

	// Tell sensor that we want to read a measure
	// Put data bus to 0
	gpio_pin_output(pin);
	gpio_pin_clr(pin);
	delay(18);

	// Configure pin as input and enable pull-up
	gpio_pin_input(pin);
	gpio_pin_pullup(pin);

	// Wait response from sensor 1 -> 0 -> 1 -> 0
	elapsed = dht11_bus_monitor(pin, 1, 50);if (elapsed == -1) goto timeout;
	elapsed = dht11_bus_monitor(pin, 0, 90);if (elapsed == -1) goto timeout;
	elapsed = dht11_bus_monitor(pin, 1, 90);if (elapsed == -1) goto timeout;

	// Wait for first bit
	for(bit=0;bit < 40;bit++) {
		// Wait for bit 0 -> 1 -> 0
		elapsed = dht11_bus_monitor(pin, 0, 60);if (elapsed == -1) goto timeout;
		elapsed = dht11_bus_monitor(pin, 1, 80);if (elapsed == -1) goto timeout;

		if (elapsed > 60) {
			data[byte] |= (1 << cnt);
		}

		if (cnt == 0) {
			cnt = 7;
			byte++;
		} else {
			cnt--;
		}
	}

	dht11_bus_monitor(pin, 1, 80);if (elapsed == -1) goto timeout;

	// Check CRC
	uint8_t crc = 0;
	crc += data[0];
	crc += data[1];
	crc += data[2];
	crc += data[3];

	if (crc != data[4]) {
		portENABLE_INTERRUPTS();
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_INVALID_DATA, "crc");
	}

	goto exit;

timeout:
	portENABLE_INTERRUPTS();
	return driver_error(SENSOR_DRIVER, SENSOR_ERR_TIMEOUT, NULL);

exit:
	portENABLE_INTERRUPTS();

	values[0].floatd.value = (float)(data[2]);
	values[1].floatd.value = (float)(data[0]);


	// Next value can get in 1 seconds
	gettimeofday(&unit->next, NULL);
	unit->next.tv_sec += 1;

	return NULL;
}

#endif
#endif

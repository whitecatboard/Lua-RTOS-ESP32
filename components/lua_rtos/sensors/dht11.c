/*
 * Lua RTOS, DHT11 sensor (temperature & humidity)
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

#include "dht11.h"

#include "freertos/FreeRTOS.h"
#include "esp_attr.h"

#include <time.h>

#include <sys/driver.h>
#include <sys/delay.h>
#include <sys/sleep.h>

#include <drivers/gpio.h>
#include <drivers/sensor.h>

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) dht11_sensor = {
	.id = "DHT11",
	.interface = GPIO_INTERFACE,
	.data = {
		{.id = "temperature", .type = SENSOR_DATA_INT},
		{.id = "humidity"   , .type = SENSOR_DATA_INT},
	},
	.setup = dht11_setup,
	.acquire = dht11_acquire
};

/*
 * Helper functions
 */
static void dht11_bus_monitor(int pin, uint8_t level, uint8_t *elapsed) {
	uint8_t val;
	unsigned start, end;

	// Get start time
	start = xthal_get_ccount();
	end = start;

	gpio_pin_get(pin, &val);
	while (val == level) {
		end = xthal_get_ccount();
		gpio_pin_get(pin, &val);
	}

	end = xthal_get_ccount();

	*elapsed = (uint8_t)((end - start) / (CPU_HZ / (1000000 * (CPU_HZ / CORE_TIMER_HZ))));
}

/*
 * Operation functions
 */
driver_error_t *dht11_setup(sensor_instance_t *unit) {
	return NULL;
}

driver_error_t *dht11_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	int8_t data[5] = {0,0,0,0,0}; // DHT11 returns 5 byte of date in each transfer
	uint8_t byte = 0;			  // Current byte transferred by sensor
	uint8_t cnt = 7;			  // Current bit into current byte transferred by sensor
	int bit = 0;                  // Current transferred bit by sensor

	uint8_t level; // Current data line logical level (0 / 1)
	uint8_t elapsed; // Elapsed time in usecs between level transitions 0 to 1 / 1 to 0

	// Get pin from instance
	uint8_t pin = unit->setup.gpio.gpio;

	usleep(200000);

	portDISABLE_INTERRUPTS();

	gpio_pin_output(pin);
	gpio_pin_clr(pin);
	delay(18);

	gpio_pin_input(pin);

	// Current level
	level = 1;

	// Wait for response
	dht11_bus_monitor(pin, level, &elapsed);level = !level;
	dht11_bus_monitor(pin, level, &elapsed);level = !level;
	dht11_bus_monitor(pin, level, &elapsed);level = !level;

	for(bit=0;bit<40;bit++) {
		dht11_bus_monitor(pin, level, &elapsed);level = !level;

		dht11_bus_monitor(pin, level, &elapsed);level = !level;
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

	if (((data[0] + data[1] + data[2] + data[3]) & 0xff) != data[4]) {
		portENABLE_INTERRUPTS();

		// TODO CHECKSUM ERROR
		return NULL;
	}

	portENABLE_INTERRUPTS();

	values[0].integerd.value = data[2];
	values[1].integerd.value = data[0];

	return NULL;
}

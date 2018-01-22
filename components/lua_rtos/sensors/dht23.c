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
 * Lua RTOS, DHT23 sensor (temperature & humidity)
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_DHT23


#include "freertos/FreeRTOS.h"
#include "esp_attr.h"

#include <time.h>

#include <sys/driver.h>
#include <sys/delay.h>
#include <sys/sleep.h>

#include <drivers/gpio.h>
#include <drivers/sensor.h>

driver_error_t *dht23_setup(sensor_instance_t *unit);
driver_error_t *dht23_acquire(sensor_instance_t *unit, sensor_value_t *values);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) dht23_sensor = {
	.id = "DHT23",
	.interface = {
		{.type = GPIO_INTERFACE},
	},
	.data = {
		{.id = "temperature", .type = SENSOR_DATA_FLOAT},
		{.id = "humidity"   , .type = SENSOR_DATA_FLOAT},
	},
	.setup = dht23_setup,
	.acquire = dht23_acquire
};

/*
 * Helper functions
 */
static int dht23_bus_monitor(uint8_t pin, uint8_t level, int16_t timeout) {
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
driver_error_t *dht23_setup(sensor_instance_t *unit) {
	// Get pin from instance
	uint8_t pin = unit->setup[0].gpio.gpio;

	// Configure pin as input(pull-up enable)
	gpio_pin_input(pin);
	gpio_pin_pullup(pin);

	return NULL;
}

driver_error_t *dht23_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	uint8_t data[5] = {0,0,0,0,0}; // dht23 returns 5 byte of date in each transfer
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
	delay(1);

	// Configure pin as input and enable pull-up
	gpio_pin_input(pin);
	gpio_pin_pullup(pin);

	// Wait response from sensor 1 -> 0 -> 1 -> 0
	elapsed = dht23_bus_monitor(pin, 1, 30);if (elapsed == -1) goto timeout;
	elapsed = dht23_bus_monitor(pin, 0, 90);if (elapsed == -1) goto timeout;
	elapsed = dht23_bus_monitor(pin, 1, 90);if (elapsed == -1) goto timeout;

	// Wait for first bit
	for(bit=0;bit < 40;bit++) {
		// Wait for bit 0 -> 1 -> 0
		elapsed = dht23_bus_monitor(pin, 0, 80);if (elapsed == -1) goto timeout;
		elapsed = dht23_bus_monitor(pin, 1, 80);if (elapsed == -1) goto timeout;

		if (elapsed >= 70) {
			data[byte] |= (1 << cnt);
		}

		if (cnt == 0) {
			cnt = 7;
			byte++;
		} else {
			cnt--;
		}
	}

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

	values[0].floatd.value = (float)(((uint16_t)data[2]) << 8 | (uint16_t)data[3]) / 10.0;

	// Apply temperature sing
	if (data[2] & 0b1000000) {
		values[0].floatd.value = -1 * values[0].floatd.value;
	}

	values[1].floatd.value = (float)(((uint16_t)data[0]) << 8 | (uint16_t)data[1]) / 10.0;

	// Next value can get in 2 seconds
	gettimeofday(&unit->next, NULL);
	unit->next.tv_sec += 2;

	return NULL;
}

#endif
#endif

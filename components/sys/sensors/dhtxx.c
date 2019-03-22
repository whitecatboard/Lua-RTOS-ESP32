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
 * Lua RTOS, DHT sensors common functions
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_DHT11 || CONFIG_LUA_RTOS_USE_SENSOR_DHT22 || CONFIG_LUA_RTOS_USE_SENSOR_DHT23

#include "freertos/FreeRTOS.h"

#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <drivers/rmt.h>

static int dhtxx_bus_monitor(uint8_t pin, uint8_t level, int16_t timeout) {
    uint32_t start, end, elapsed;
    uint8_t val;

    // Get start time
    start = xthal_get_ccount();

    gpio_pin_get(pin, &val);

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

driver_error_t *dhtxx_setup(sensor_instance_t *unit) {
	driver_error_t *error;

    // Get pin from instance
    uint8_t pin = unit->setup[0].gpio.gpio;

	// The preferred implementation uses the RMT to avoid disabling interrupts during
	// the acquire process. If there a not RMT channels available we use the bit bang
	// implementation.
    int rmt_device;

    error = rmt_setup_rx(pin, RMTPulseRangeUSEC, &rmt_device);
    if (!error) {
		// Use RMT implementation
		unit->args = (void *)((uint32_t)rmt_device);
    } else {
    	// Not possible
    	free(error);

        // Use software implementation
        unit->args = (void *)0xffffffff;
    }

#if 0
	rmt_config_t rmt_rx;
	uint8_t channel;

	rmt_rx.gpio_num = pin;
	rmt_rx.clk_div = (APB_CLK_FREQ / 1000000); // Count in microseconds
	rmt_rx.mem_block_num = 1;
	rmt_rx.rmt_mode = RMT_MODE_RX;
	rmt_rx.rx_config.filter_en = 1;
	rmt_rx.rx_config.filter_ticks_thresh = 200;
	rmt_rx.rx_config.idle_threshold = 200;

	// Configure the RMT, and get a free channel for read
	if (rmt_config_one(&rmt_rx, &channel) == 0) {
		// Install driver for channel
		if (rmt_driver_install(channel, 1000, 0) == 0) {
			// Use RMT implementation
			unit->args = (void *)((uint32_t)channel);

			rmt_rx_start((rmt_channel_t)unit->args, 1);
			rmt_rx_stop((rmt_channel_t)unit->args);

			return NULL;
		}
	}
#endif

    // Release data bus
    gpio_pin_input(pin);
    gpio_pin_pullup(pin);

    return NULL;
}

driver_error_t *dhtxx_acquire(sensor_instance_t *unit, uint8_t mdt, uint8_t *data) {
    uint8_t retries = 0; // In case of problems we will retry the acquire a number of times
    uint8_t byte = 0;    // Current byte transferred by sensor
    int8_t  bit = 0;     // Current transferred bit by sensor

    // Get pin from instance
    uint8_t pin = unit->setup[0].gpio.gpio;

retry:
	// We initialize the data in this way to detect a CRC error in case of problems: sensor
	// not connected, bad wire quality, interferences ...
	data[0] = data[1] = data[2] = data[3] = 0xff;
	data[4] = 0x00;

    if ((uint32_t)unit->args != 0xffffffff) {
    	driver_error_t *error;
    	rmt_item_t *items;
    	rmt_item_t *item;

		// Take data bus and set to low to inform the sensor that we want to acquire data
		gpio_pin_output(pin);
		gpio_pin_clr(pin);
		delay(mdt);

		// Release data bus
		gpio_pin_input(pin);

		// Read 41 items, timeout 100 milliseconds
    	error = rmt_rx((int)unit->args, 41, 100, &items);
    	if (!error) {
    		item = items;

			// Skip first item, because it corresponds to the pulse sent by the
			// sensor to indicate that it will start to send the data.
    		item++;

			// The sensor returns 5 bytes. Each bit is encoded as a low to high transition as
    		// follows:
    		//
    		// bit 0: L 54 +/- tolerance usecs, H 24 +/- tolerance usecs
    		// bit 1: L 54 +/- tolerance usecs, H 70 +/- tolerance usecs
    		int t0, t1;

			for(byte=0;byte < 5;byte++) {
				data[byte] = 0;

				for(bit = 7;bit >= 0;bit--) {
					if (item->level0 == 0) {
						t0 = item->duration0;
					} else {
						t1 = item->duration0;
					}

					if (item->level1 == 0) {
						t0 = item->duration1;
					} else {
						t1 = item->duration1;
					}

					if ((t0 >= 54 - DHTXX_TOLERANCE) && (t0 <= 54 + DHTXX_TOLERANCE)) {
						if ((t1 >= 70 - DHTXX_TOLERANCE) && (t1 <= 70 + DHTXX_TOLERANCE)) {
							data[byte] |= (1 << bit);
						} else if ((t1 >= 24 - DHTXX_TOLERANCE) && (t1 <= 24 + DHTXX_TOLERANCE)) {
							data[byte] &= ~(1 << bit);
						} else {
				    		free(items);

				    	    retries++;
				    	    if (retries < 5) {
								goto retry;
				    	    }

							// Error
					        return driver_error(SENSOR_DRIVER, SENSOR_ERR_INVALID_DATA, "unexpected bit encode");
						}
					} else {
			    		free(items);

			    		retries++;
			    	    if (retries < 5) {
							goto retry;
			    	    }

						// Error
				        return driver_error(SENSOR_DRIVER, SENSOR_ERR_INVALID_DATA, "unexpected bit encode");
					}

					item++;
				}
			}

    		free(items);
    	} else {

    	}
    } else {
        int elapsed; // Elapsed time in usecs between level transitions 0->1 / 1->0

        // Use software version
        portDISABLE_INTERRUPTS();

		// Take data bus and set to low to inform the sensor that
		// we want to acquire data
		gpio_pin_output(pin);
		gpio_pin_clr(pin);
		delay(mdt);

		// Release data bus
		gpio_pin_input(pin);
	    gpio_pin_pullup(pin);

        // Wait response from sensor 1 -> 0 -> 1 -> 0
        elapsed = dhtxx_bus_monitor(pin, 1, 100);if (elapsed == -1) goto timeout;
        elapsed = dhtxx_bus_monitor(pin, 0, 100);if (elapsed == -1) goto timeout;
        elapsed = dhtxx_bus_monitor(pin, 1, 100);if (elapsed == -1) goto timeout;

		for(byte=0;byte < 5;byte++) {
			data[byte] = 0;

			for(bit=7;bit >= 0;bit--) {
	            // Wait for bit 0 -> 1 -> 0
	            elapsed = dhtxx_bus_monitor(pin, 0, 100);if (elapsed == -1) goto timeout;
	            elapsed = dhtxx_bus_monitor(pin, 1, 100);if (elapsed == -1) goto timeout;

				data[byte] |= (((elapsed < 50)?0:1) << bit);
			}
		}
    }

    // Check CRC
    uint8_t crc = 0;
    crc += data[0];
    crc += data[1];
    crc += data[2];
    crc += data[3];

    if (crc != data[4]) {
		if ((uint32_t)unit->args == 0xffffffff) {
			portENABLE_INTERRUPTS();
		}

		retries++;
		if (retries < 5) {
			goto retry;
		}

        return driver_error(SENSOR_DRIVER, SENSOR_ERR_INVALID_DATA, "crc");
    }

    goto exit;

timeout:
	if ((uint32_t)unit->args == 0xffffffff) {
		portENABLE_INTERRUPTS();
	}

    retries++;
    if (retries < 5) {
    		goto retry;
    }

    return driver_error(SENSOR_DRIVER, SENSOR_ERR_TIMEOUT, NULL);

exit:
	if ((uint32_t)unit->args == 0xffffffff) {
		portENABLE_INTERRUPTS();
	}

	return NULL;
}

driver_error_t *dhtxx_unsetup(sensor_instance_t *unit) {
	if ((uint32_t)unit->args != 0xffffffff) {
		rmt_driver_uninstall((rmt_channel_t)unit->args);
	}

	return NULL;
}
#endif
#endif

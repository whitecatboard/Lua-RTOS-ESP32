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

/*
 *
 * The sensor returns 5 bytes. Each bit is encoded as follows:
 *
 * A high to low transition (start bit)
 * A low to high transition, in with it's length determines the bit value
 *
 * For all the dhtxx sensor series, we can apply the following rule to get
 * the bit value:
 *
 * - bit is 0 if high transition time is less than low transition time
 * - bit is 1 if high transition time is higher than low transition time
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_DHT11 || CONFIG_LUA_RTOS_USE_SENSOR_DHT22 || CONFIG_LUA_RTOS_USE_SENSOR_DHT23

#include "freertos/FreeRTOS.h"

#include <sys/driver.h>
#include <sys/delay.h>

#include <sensors/dhtxx.h>

#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <drivers/rmt.h>
#include <drivers/power_bus.h>

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
    driver_unit_lock_error_t *lock_error = NULL;

    // Get pin from instance
    uint8_t pin = unit->setup[0].gpio.gpio;

    // By default, use software implementation
    unit->args = (void *)0xffffffff;

#if CONFIG_LUA_RTOS_LUA_USE_RMT
    driver_error_t *error;

    // The preferred implementation uses the RMT to avoid disabling interrupts during
    // the acquire process. If there a not RMT channels available we use the bit bang
    // implementation.
    int rmt_device;

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    // At this point pin is locked by sensor driver. In this case we need to unlock the
    // resource first, because maybe we can use RMT.
    driver_unlock(SENSOR_DRIVER, unit->unit, GPIO_DRIVER, pin);
#endif

    error = rmt_setup_tx(pin, RMTPulseRangeUSEC, RMTIdleZ, NULL, &rmt_device);
    if (!error) {
        error = rmt_setup_rx(pin, RMTPulseRangeUSEC, 10, 100, &rmt_device);
        if (!error) {
    #if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
            // Lock RMT for this sensor (pin is locked by RMT)
            if ((lock_error = driver_lock(SENSOR_DRIVER, unit->unit, RMT_DRIVER, rmt_device, DRIVER_ALL_FLAGS, unit->sensor->id))) {
                free(lock_error);
            } else {
                // Use RMT implementation
                unit->args = (void *)((uint32_t)rmt_device);
            }
    #endif
        } else {
            free(error);
        }
    } else {
        free(error);
    }

    if ((uint32_t)unit->args == 0xffffffff) {
#endif
        // Use bit bang
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
        // Lock GPIO for this sensor
        if ((lock_error = driver_lock(SENSOR_DRIVER, unit->unit, GPIO_DRIVER, pin, DRIVER_ALL_FLAGS, unit->sensor->id))) {
            return driver_lock_error(SENSOR_DRIVER, lock_error);
        }
#endif
#if CONFIG_LUA_RTOS_LUA_USE_RMT
    }
#endif

    // Release data bus
    gpio_pin_input(pin);
    gpio_pin_pullup(pin);

    return NULL;
}

driver_error_t *dhtxx_postsetup(sensor_instance_t *unit) {
    // Datasheet says:
    //
    // When power is supplied to the sensor, do not send any
    // instruction to the sensor in within one second in order to pass the unstable status

#if (CONFIG_LUA_RTOS_POWER_BUS_PIN >= 0)
    if (pwbus_uptime() < 1000) {
        delay(1000);
    }
#endif

    return NULL;
}

driver_error_t *dhtxx_acquire(sensor_instance_t *unit, uint32_t rdelay, uint32_t atime, uint8_t *data) {
    uint8_t retries = 0; // In case of problems we will retry the acquire a number of times
    uint8_t byte = 0;    // Current byte transferred by sensor
    int8_t  bit = 0;     // Current transferred bit by sensor
    int t0;              // High to low transition length in usecs
    int t1;              // Low to high transition length in usecs

    // Get pin from instance
    uint8_t pin = unit->setup[0].gpio.gpio;

retry:
    // We initialize the data in this way to detect a CRC error in case of problems: sensor
    // not connected, bad wire quality, interferences ...
    data[0] = data[1] = data[2] = data[3] = 0xff;
    data[4] = 0x00;

#if CONFIG_LUA_RTOS_LUA_USE_RMT
    if ((uint32_t)unit->args != 0xffffffff) {
        driver_error_t *error;
        rmt_item_t *item;

        // First send, request pulse
        rmt_item_t buffer[41];

        buffer[0].level0 = 0;
        buffer[0].duration0 = rdelay * 1000;
        buffer[0].level1 = 1;
        buffer[0].duration1 = 40;

        error = rmt_tx_rx((int)unit->args, buffer, 1, buffer, 41, 100000);
        if (!error) {
            item = buffer;

            // Skip first item, because it corresponds to the pulse sent by the
            // sensor to indicate that it will start to send the data.
            item++;

            for(byte=0;byte < 5;byte++) {
                data[byte] = 0;

                for(bit = 7;bit >= 0;bit--) {
                    if (item->level0 == 0) {
                        t0 = item->duration0;
                        t1 = item->duration1;
                    } else {
                        t0 = item->duration1;
                        t1 = item->duration0;
                    }

                    if (t1 > t0) {
                        data[byte] |= (1 << bit);
                    }

                    item++;
                }
            }
        } else {
            return error;
        }
    } else {
#endif
        // Use software version
        portDISABLE_INTERRUPTS();

        // Inform the sensor that we want to acquire data
        gpio_pin_output(pin);
        gpio_pin_clr(pin);
        delay(rdelay);

        // Receive data
        gpio_pin_input(pin);
        gpio_pin_pullup(pin);

        // Wait response from sensor 1 -> 0 -> 1 -> 0
        t1 = dhtxx_bus_monitor(pin, 1, 100);if (t1 == -1) goto timeout;
        t0 = dhtxx_bus_monitor(pin, 0, 100);if (t0 == -1) goto timeout;
        t1 = dhtxx_bus_monitor(pin, 1, 100);if (t1 == -1) goto timeout;

        for(byte=0;byte < 5;byte++) {
            data[byte] = 0;

            for(bit=7;bit >= 0;bit--) {
                // Wait for bit 0 -> 1 -> 0
                t0 = dhtxx_bus_monitor(pin, 0, 100);if (t0 == -1) goto timeout;
                t1 = dhtxx_bus_monitor(pin, 1, 100);if (t1 == -1) goto timeout;

                data[byte] |= ((t1 > t0) << bit);
            }
        }
#if CONFIG_LUA_RTOS_LUA_USE_RMT
    }
#endif

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
            delay(atime);
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
        delay(atime);
        goto retry;
    }

    return driver_error(SENSOR_DRIVER, SENSOR_ERR_TIMEOUT, NULL);

exit:
    if ((uint32_t)unit->args == 0xffffffff) {
        portENABLE_INTERRUPTS();
    }

    gettimeofday(&unit->next, NULL);
    unit->next.tv_sec += atime / 1000;

    return NULL;
}

driver_error_t *dhtxx_unsetup(sensor_instance_t *unit) {
#if CONFIG_LUA_RTOS_LUA_USE_RMT
	if ((uint32_t)unit->args != 0xffffffff) {
        rmt_unsetup_rx((int)unit->args);

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
        driver_unlock(SENSOR_DRIVER, unit->unit, RMT_DRIVER, (uint32_t)unit->args);
#endif
    } else {
#endif
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
        driver_unlock(SENSOR_DRIVER, unit->unit, GPIO_DRIVER, unit->setup[0].gpio.gpio);
#endif
#if CONFIG_LUA_RTOS_LUA_USE_RMT
    }
#endif

    return NULL;
}
#endif
#endif

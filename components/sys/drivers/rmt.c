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
 * Lua RTOS, RMT driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_RMT

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"

#include <string.h>

#include <soc/soc.h>
#include <driver/rmt.h>

#include <drivers/rmt.h>
#include <drivers/cpu.h>
#include <drivers/gpio.h>

#include <sys/driver.h>
#include <sys/mutex.h>
#include <sys/syslog.h>

static void rmt_init();
static struct mtx mtx;

// Register driver and messages
DRIVER_REGISTER_BEGIN(RMT,rmt,CPU_LAST_RMT_CH - CPU_FIRST_RMT_CH + 1,rmt_init,NULL);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidPulseRange, "invalid pulse range", RMT_ERR_INVALID_PULSE_RANGE);
	DRIVER_REGISTER_ERROR(RMT, rmt, NoMoreRMT, "no more RMT channels available", RMT_ERR_NO_MORE_RMT);
	DRIVER_REGISTER_ERROR(RMT, rmt, InvalidPin, "invalid pin", RMT_ERR_INVALID_PIN);
	DRIVER_REGISTER_ERROR(RMT, rmt, Timeout, "timeout", RMT_ERR_TIMEOUT);
	DRIVER_REGISTER_ERROR(RMT, rmt, Unexpected, "unexpected", RMT_ERR_UNEXPECTED);
	DRIVER_REGISTER_ERROR(RMT, rmt, InvalidIdleLevel, "invalid idle level", RMT_ERR_INVALID_IDLE_LEVEL);
DRIVER_REGISTER_END(RMT,rmt,CPU_LAST_RMT_CH - CPU_FIRST_RMT_CH + 1,rmt_init,NULL);

/*
 * Helper functions
 */

static void rmt_init() {
    mtx_init(&mtx, NULL, NULL, 0);
}

typedef struct {
	int8_t pin;
	rmt_status_t status;
	struct mtx mtx;
	RingbufHandle_t rb;

	uint8_t rx_config;
	struct {
		rmt_pulse_range_t range;
		float scale;
	} rx;

	uint8_t tx_config;
	struct {
		uint8_t in_loop;
		rmt_idle_level idle_level;
		rmt_pulse_range_t range;
		float scale;
	} tx;
} rmt_device_t;

static rmt_device_t *devices = NULL;

/*
 * Operation functions
 */

static int rmt_get_channel_by_pin(int pin) {
    int i;

    for (i = CPU_FIRST_RMT_CH; i < CPU_LAST_RMT_CH; i++) {
    	if (devices[i].pin == pin) {
    		return i;
    	}
    }

    return -1;
}

static int rmt_get_free_channel() {
    int i;

    for (i = CPU_FIRST_RMT_CH; i < CPU_LAST_RMT_CH; i++) {
    	if (devices[i].pin == -1) {
    		return i;
    	}
    }

    return -1;
}

static int create_devices() {
	if (devices == NULL) {
		devices = calloc(CPU_LAST_RMT_CH - CPU_FIRST_RMT_CH + 1, sizeof(rmt_device_t));
		if (!devices) {
			return -1;
		}

		int i;

	    for (i = CPU_FIRST_RMT_CH; i < CPU_LAST_RMT_CH; i++) {
	    	devices[i].pin = -1;
	    }
	}

	return 0;
}

static driver_error_t *setup_rx(int deviceid) {
	uint8_t channel = deviceid;
	rmt_config_t rmt_rx;

	// If last mode was RX exit immediately
	if (devices[channel].status == RMTStatusRX) {
		return NULL;
	}

	rmt_rx.channel = channel;
	rmt_rx.gpio_num = devices[channel].pin;

	// Set divider
	if (devices[channel].rx.range == RMTPulseRangeNSEC) {
		// Count in nanoseconds, but with APB_CLK_FREQ resolution of RMT is 12.5 nanoseconds
		rmt_rx.clk_div = 1;
		devices[channel].rx.scale = 12.5;
	} else if (devices[channel].rx.range == RMTPulseRangeUSEC) {
		// Count in microseconds
		rmt_rx.clk_div = (uint8_t)(APB_CLK_FREQ / 1000000UL);
		devices[channel].rx.scale = 1;
	} else if (devices[channel].rx.range == RMTPulseRangeMSEC) {
		// Count in milliseconds
		rmt_rx.clk_div = (uint8_t)(APB_CLK_FREQ / 1000UL);
		devices[channel].rx.scale = 1;
	}

	rmt_rx.mem_block_num = 1;
	rmt_rx.rmt_mode = RMT_MODE_RX;
	rmt_rx.rx_config.filter_en = 1;
	rmt_rx.rx_config.filter_ticks_thresh = 200;
	rmt_rx.rx_config.idle_threshold = 200;

	// Configure the RMT
	esp_err_t ret;

	rmt_driver_uninstall(channel);

	if (rmt_config(&rmt_rx) != ESP_OK) {
		// Should not happen
		return driver_error(RMT_DRIVER, RMT_ERR_UNEXPECTED, "1");
	}

	if ((ret = rmt_driver_install(channel, 1000, 0)) != ESP_OK) {
		if (ret == ESP_ERR_NO_MEM) {
			return driver_error(RMT_DRIVER, RMT_ERR_NOT_ENOUGH_MEMORY, NULL);
		} else {
			return driver_error(RMT_DRIVER, RMT_ERR_UNEXPECTED, "2");
		}
	}

    // Get the ring buffer to receive data
    if ((rmt_get_ringbuf_handle(channel, &devices[channel].rb) != ESP_OK) || (devices[channel].rb == NULL)) {
		// Should not happen
		return driver_error(RMT_DRIVER, RMT_ERR_UNEXPECTED, "3");
    }

    devices[channel].status = RMTStatusRX;

    return NULL;

}

static driver_error_t *setup_tx(int deviceid) {
	uint8_t channel = deviceid;
	rmt_config_t rmt_tx;

	// If last mode was TX exit immediately
	if (devices[channel].status == RMTStatusTX) {
		return NULL;
	}

	rmt_tx.channel = channel;
	rmt_tx.gpio_num = devices[channel].pin;

	// Set divider
	if (devices[channel].tx.range == RMTPulseRangeNSEC) {
		// Count in nanoseconds, but with APB_CLK_FREQ resolution of RMT is 12.5 nanoseconds
		rmt_tx.clk_div = 1;
		devices[channel].tx.scale = 12.5;
	} else if (devices[channel].tx.range == RMTPulseRangeUSEC) {
		// Count in microseconds
		rmt_tx.clk_div = (uint8_t)(APB_CLK_FREQ / 1000000UL);
		devices[channel].tx.scale = 1;
	} else if (devices[channel].rx.range == RMTPulseRangeMSEC) {
		// Count in milliseconds
		rmt_tx.clk_div = (uint8_t)(APB_CLK_FREQ / 1000UL);
		devices[channel].tx.scale = 1;
	}

	rmt_tx.mem_block_num = 3;
	rmt_tx.rmt_mode = RMT_MODE_TX;

	rmt_tx.tx_config.loop_en = devices[channel].tx.in_loop;
	rmt_tx.tx_config.carrier_en = 0;

	if (devices[channel].tx.idle_level == RMTIdleZ) {
		rmt_tx.tx_config.idle_output_en = 0;
	} else if (devices[channel].tx.idle_level == RMTIdleL) {
		rmt_tx.tx_config.idle_output_en = 1;
		rmt_tx.tx_config.idle_level = 0;
	} else if (devices[channel].tx.idle_level == RMTIdleH) {
		rmt_tx.tx_config.idle_output_en = 1;
		rmt_tx.tx_config.idle_level = 1;
	}

	// Configure the RMT
	esp_err_t ret;

	//rmt_driver_uninstall(channel);

	if (rmt_config(&rmt_tx) != ESP_OK) {
		// Should not happen
		return driver_error(RMT_DRIVER, RMT_ERR_UNEXPECTED, "4");
	}

	if ((ret = rmt_driver_install(channel, 0, 0)) != ESP_OK) {
		if (ret == ESP_ERR_NO_MEM) {
			return driver_error(RMT_DRIVER, RMT_ERR_NOT_ENOUGH_MEMORY, NULL);
		} else {
			return driver_error(RMT_DRIVER, RMT_ERR_UNEXPECTED, "5");
		}
	}

    devices[channel].status = RMTStatusTX;

    return NULL;

}

driver_error_t *rmt_setup_rx(int pin, rmt_pulse_range_t range, int *deviceid) {
	// Sanity checks
    if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << pin))) {
        return driver_error(GPIO_DRIVER, RMT_ERR_INVALID_PIN, NULL);
    }

    if (range >= RMTPulseRangeMAX) {
        return driver_error(RMT_DRIVER, RMT_ERR_INVALID_PULSE_RANGE, NULL);
	}

	// Setup
	mtx_lock(&mtx);

	// Create device structure, if required
	if (create_devices() < 0) {
		mtx_unlock(&mtx);

        return driver_error(RMT_DRIVER, RMT_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Find an existing channel to pin
	int8_t channel = rmt_get_channel_by_pin(pin);
	if (channel < 0) {
		// Device not found, so get a free device
		channel = rmt_get_free_channel();
		if (channel < 0) {
			mtx_unlock(&mtx);

			// No more channels
	        return driver_error(RMT_DRIVER, RMT_ERR_NO_MORE_RMT, NULL);
		}
	}

	*deviceid = channel;

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	// Lock resources
    driver_unit_lock_error_t *lock_error = NULL;

    if ((lock_error = driver_lock(RMT_DRIVER, channel, GPIO_DRIVER, pin, DRIVER_ALL_FLAGS, NULL))) {
		mtx_unlock(&mtx);

        return driver_lock_error(RMT_DRIVER, lock_error);
    }
#endif

	// Store device information
	devices[channel].pin = pin;
	devices[channel].rx.range = range;
    devices[channel].rx_config = 1;

	// Create mutex for channel
    mtx_init(&devices[channel].mtx, NULL, NULL, 0);

	mtx_unlock(&mtx);

    syslog(LOG_INFO,
           "rmt%u rx at pin %s%d", channel,
           gpio_portname(pin), gpio_name(pin));

	return NULL;
}

driver_error_t *rmt_setup_tx(int pin, rmt_pulse_range_t pulse_range, rmt_idle_level idle_level, int *deviceid) {
	// Sanity checks
    if (!(GPIO_ALL_IN & (GPIO_BIT_MASK << pin))) {
        return driver_error(GPIO_DRIVER, RMT_ERR_INVALID_PIN, NULL);
    }

    if (idle_level >= RMTIdleMAX) {
        return driver_error(RMT_DRIVER, RMT_ERR_INVALID_IDLE_LEVEL, NULL);
	}

	// Setup
	mtx_lock(&mtx);

	// Create device structure, if required
	if (create_devices() < 0) {
		mtx_unlock(&mtx);

        return driver_error(RMT_DRIVER, RMT_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Find an existing channel to pin
	int8_t channel = rmt_get_channel_by_pin(pin);
	if (channel < 0) {
		// Device not found, so get a free device
		channel = rmt_get_free_channel();
		if (channel < 0) {
			mtx_unlock(&mtx);

			// No more channels
	        return driver_error(RMT_DRIVER, RMT_ERR_NO_MORE_RMT, NULL);
		}
	}

	*deviceid = channel;

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	// Lock resources
    driver_unit_lock_error_t *lock_error = NULL;

    if ((lock_error = driver_lock(RMT_DRIVER, channel, GPIO_DRIVER, pin, DRIVER_ALL_FLAGS, NULL))) {
		mtx_unlock(&mtx);

        return driver_lock_error(RMT_DRIVER, lock_error);
    }
#endif

	// Store device information
	devices[channel].pin = pin;
	devices[channel].tx.range = pulse_range;
	devices[channel].tx.idle_level = idle_level;
    devices[channel].tx_config = 1;

	// Create mutex for channel
    mtx_init(&devices[channel].mtx, NULL, NULL, 0);

	mtx_unlock(&mtx);

    syslog(LOG_INFO,
           "rmt%u tx at pin %s%d", channel,
           gpio_portname(pin), gpio_name(pin));

	return NULL;
}

driver_error_t *rmt_rx(int deviceid, uint32_t pulses, uint32_t timeout, rmt_item_t **buffer) {
	driver_error_t *error = NULL;
    uint8_t channel = deviceid; // RMT channel

    *buffer = NULL;

    // Allocate output buffer
    rmt_item_t *buff = calloc(pulses, sizeof(rmt_item32_t));
    if (!buff) {
    	return driver_error(RMT_DRIVER, RMT_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    mtx_lock(&devices[channel].mtx);

    // Setup for RX
    error = setup_rx(deviceid);
    if (error) {
        mtx_unlock(&devices[channel].mtx);
    	free(buff);
    	return error;
    }

	// Start RMT and receive data
    if (rmt_rx_start(channel, 1) != ESP_OK){
    	// Should not happen
		return driver_error(RMT_DRIVER, RMT_ERR_UNEXPECTED, "6");
    }

	// Wait for data
    rmt_item_t *cbuff;    // Output buffer current position
    rmt_item32_t *ritems; // Items received in current iteration
	size_t items = 0;     // Number of items received in current iteration

	cbuff = buff;

	uint32_t pending = pulses;

    while (pending > 0) {
		// Wait for data
		ritems = (rmt_item32_t*)xRingbufferReceive(devices[channel].rb, &items, timeout / portTICK_PERIOD_MS);
		if (ritems) {
			// Process only as much items of expected pulses
			items = ((items<=pulses)?items:pulses);

			// Copy to output buffer
			memcpy(cbuff, ritems, items * sizeof(rmt_item32_t));
			cbuff += items;

			pending -= items;

			// Return items to buffer
			vRingbufferReturnItem(devices[channel].rb, (void *)ritems);

			// Stop RMT
			if (rmt_rx_stop(channel) != ESP_OK) {
			    mtx_unlock(&devices[channel].mtx);

				// Should not happen
				return driver_error(RMT_DRIVER, RMT_ERR_UNEXPECTED, "7");
			}
		} else {
			// No data received, timeout
			free(buff);

			// Stop RMT
			if (rmt_rx_stop(channel) != ESP_OK) {
				// Should not happen
				return driver_error(RMT_DRIVER, RMT_ERR_UNEXPECTED, "8");
			}

	        mtx_unlock(&devices[channel].mtx);

			return driver_error(RMT_DRIVER, RMT_ERR_TIMEOUT, NULL);
		}
	}

    mtx_unlock(&devices[channel].mtx);

    // Need scale?
    if (devices[channel].rx.scale != 1.0) {
    	rmt_item_t *cbuffer = buff;
    	int i;

    	for(i = 0; i < pulses;i++) {
    		cbuffer->duration0 *= devices[channel].tx.scale;
    		cbuffer->duration1 *= devices[channel].tx.scale;

    		cbuffer++;
    	}
    }

    // Return output buffer
	*buffer = buff;

	return NULL;
}

driver_error_t *rmt_tx(int deviceid, rmt_item_t *buffer, size_t pulses) {
	driver_error_t *error = NULL;
    uint8_t channel = deviceid; // RMT channel

    mtx_lock(&devices[channel].mtx);

    // Setup for TX
    error = setup_tx(deviceid);
    if (error) {
        mtx_unlock(&devices[channel].mtx);
    	return error;
    }

    // Need scale?
    if (devices[channel].tx.scale != 1.0) {
    	rmt_item_t *cbuffer = buffer;
    	int i;

    	for(i = 0; i < pulses;i++) {
    		cbuffer->duration0 /= devices[channel].tx.scale;
    		cbuffer->duration1 /= devices[channel].tx.scale;

    		cbuffer++;
    	}
    }

    if (rmt_write_items(channel, (rmt_item32_t *)buffer, pulses, 1) != ESP_OK) {
        mtx_unlock(&devices[channel].mtx);

        // Should not happen
		return driver_error(RMT_DRIVER, RMT_ERR_UNEXPECTED, "9");
    }

    mtx_unlock(&devices[channel].mtx);

    return NULL;
}

void rmt_unsetup_tx(int deviceid) {
    uint8_t channel = deviceid; // RMT channel

    mtx_lock(&mtx);

    // Device now is not for TX
    devices[channel].tx_config = 0;

    if (!devices[channel].rx_config) {
    	// Device is also not for RX, we can free resources

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    	// Unlock resources
        driver_unlock(RMT_DRIVER, channel, GPIO_DRIVER, devices[channel].pin);
#endif

        // Free device
    	devices[channel].pin = -1;

    	// Destroy device mtx
        mtx_destroy(&devices[channel].mtx);

        // Uninstall channel
    	rmt_driver_uninstall(channel);
    }

    mtx_unlock(&mtx);

    return NULL;
}

void rmt_unsetup_rx(int deviceid) {
    uint8_t channel = deviceid; // RMT channel

    mtx_lock(&mtx);

    // Device now is not for RX
    devices[channel].rx_config = 0;

    if (!devices[channel].tx_config) {
    	// Device is also not for TX, we can free resources

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    	// Unlock resources
        driver_unlock(RMT_DRIVER, channel, GPIO_DRIVER, devices[channel].pin);
#endif

        // Free device
    	devices[channel].pin = -1;
    	devices[channel].status = RMTStatusUnknow;

    	// Destroy device mtx
        mtx_destroy(&devices[channel].mtx);

        // Uninstall channel
    	rmt_driver_uninstall(channel);
    }

    mtx_unlock(&mtx);

    return NULL;
}

#endif

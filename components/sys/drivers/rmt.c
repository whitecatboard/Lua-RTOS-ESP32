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

#include <math.h>
#include <string.h>

#include <esp_log.h>
#include <soc/soc.h>
#include <driver/rmt.h>

#include <drivers/rmt.h>
#include <drivers/cpu.h>
#include <drivers/gpio.h>

#include <sys/driver.h>
#include <sys/mutex.h>

static void rmt_init();
static struct mtx mtx;

// Register driver and messages
DRIVER_REGISTER_BEGIN(RMT,rmt,CPU_LAST_RMT_CH - CPU_FIRST_RMT_CH + 1,rmt_init,NULL);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidPulseRange, "invalid pulse range", RMT_ERR_INVALID_PULSE_RANGE);
    DRIVER_REGISTER_ERROR(RMT, rmt, NoMoreRMT, "no more channels available", RMT_ERR_NO_MORE_RMT);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidPin, "invalid pin", RMT_ERR_INVALID_PIN);
    DRIVER_REGISTER_ERROR(RMT, rmt, Timeout, "timeout", RMT_ERR_TIMEOUT);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidIdleLevel, "invalid idle level", RMT_ERR_INVALID_IDLE_LEVEL);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidTimeout, "invalid timeout", RMT_ERR_INVALID_TIMEOUT);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidFilterTicks, "invalid filter ticks", RMT_ERR_INVALID_FILTER_TICKS);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidIdleThreshold, "invalid idle threshold", RMT_ERR_INVALID_IDLE_THRESHOLD);
DRIVER_REGISTER_END(RMT,rmt,CPU_LAST_RMT_CH - CPU_FIRST_RMT_CH + 1,rmt_init,NULL);

static rmt_device_t *devices = NULL;

/*
 * Helper functions
 */

static void rmt_init() {
    mtx_init(&mtx, NULL, NULL, 0);
}

static void tx_end(rmt_channel_t channel, void *arg) {
    if (devices[channel].tx.callback) {
        devices[channel].tx.callback(channel);
    }
}

static void switch_rx(int channel) {
    // Stop transmission
    rmt_tx_stop(channel);

    // Configure RMT in reception mode, and start reception
    rmt_set_pin(channel, RMT_MODE_RX, devices[channel].pin);
    rmt_rx_start(channel, 1);
}

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

driver_error_t *rmt_setup_rx(int pin, rmt_pulse_range_t range, rmt_filter_ticks_thresh_t filter_ticks, rmt_idle_threshold_t idle_threshold, int *deviceid) {
    // Sanity checks
    if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << pin))) {
        return driver_error(GPIO_DRIVER, RMT_ERR_INVALID_PIN, NULL);
    }

    if (range >= RMTPulseRangeMAX) {
        return driver_error(RMT_DRIVER, RMT_ERR_INVALID_PULSE_RANGE, NULL);
    }

    if ((filter_ticks < 0) || (filter_ticks > 0xff)) {
        return driver_error(RMT_DRIVER, RMT_ERR_INVALID_FILTER_TICKS, NULL);
    }

    if ((idle_threshold < 0) || (idle_threshold > 0xffff)) {
        return driver_error(RMT_DRIVER, RMT_ERR_INVALID_IDLE_THRESHOLD, NULL);
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

    // Avoid to setup if yet setup for RX
    if (devices[channel].rx_config) {
        mtx_unlock(&mtx);
        return NULL;
    }

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    // Lock resources
    driver_unit_lock_error_t *lock_error = NULL;

    if ((lock_error = driver_lock(RMT_DRIVER, channel, GPIO_DRIVER, pin, DRIVER_ALL_FLAGS, NULL))) {
        mtx_unlock(&mtx);

        return driver_lock_error(RMT_DRIVER, lock_error);
    }
#endif

    // Configure the RMT
    rmt_config_t rmt_rx;
    esp_err_t ret;

    rmt_rx.channel = channel;
    rmt_rx.gpio_num = pin;
    rmt_rx.rmt_mode = RMT_MODE_RX;

    // Set divider
    if (range == RMTPulseRangeNSEC) {
        // Count in nanoseconds, but with APB_CLK_FREQ resolution of RMT is 12.5 nanoseconds
        rmt_rx.clk_div = 1;
        devices[channel].rx.scale = 12.5;
    } else if (range == RMTPulseRangeUSEC) {
        // Count in microseconds
        rmt_rx.clk_div = (uint8_t)(APB_CLK_FREQ / 1000000UL);
        devices[channel].rx.scale = 1;
    } else if (range == RMTPulseRangeMSEC) {
        // Count in milliseconds
        rmt_rx.clk_div = (uint8_t)(APB_CLK_FREQ / 1000UL);
        devices[channel].rx.scale = 1;
    }

    rmt_rx.mem_block_num = 1;

    if (filter_ticks > 0) {
        rmt_rx.rx_config.filter_en = 1;
        rmt_rx.rx_config.filter_ticks_thresh = filter_ticks;
    } else{
        rmt_rx.rx_config.filter_en = 0;
        rmt_rx.rx_config.filter_ticks_thresh = 0;
    }

    rmt_rx.rx_config.idle_threshold = idle_threshold / devices[channel].rx.scale;

    assert(rmt_config(&rmt_rx) == ESP_OK);

    // Be sure that driver is not installed for channel, but first time esp-idf shows an error.
    // Unfortunately esp-idf hasn't an API to check if driver installed, so we avoid esp-idf to
    // show the error.
    esp_log_level_set("rmt", ESP_LOG_NONE);
    rmt_driver_uninstall(channel);
    esp_log_level_set("rmt", CONFIG_LOG_DEFAULT_LEVEL);

    if ((ret = rmt_driver_install(channel, 1000, ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED)) != ESP_OK) {
        if (ret == ESP_ERR_NO_MEM) {
            return driver_error(RMT_DRIVER, RMT_ERR_NOT_ENOUGH_MEMORY, NULL);
        } else {
            assert(ret != ESP_ERR_INVALID_STATE);
        }
    }

    // Get the ring buffer to receive data
    assert((rmt_get_ringbuf_handle(channel, &devices[channel].rb) == ESP_OK) && (devices[channel].rb != NULL));

    // Create mutex for channel, if not yet created
    if (!mtx_inited(&devices[channel].mtx)) {
        mtx_init(&devices[channel].mtx, NULL, NULL, 0);
    }

    devices[channel].pin = pin;
    devices[channel].rx.range = range;
    *deviceid = channel;

    mtx_unlock(&mtx);

    return NULL;
}

driver_error_t *rmt_setup_tx(int pin, rmt_pulse_range_t range, rmt_idle_level idle_level, rmt_callback_t callback, int *deviceid) {
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

    // Avoid to setup if yet setup for TX
    if (devices[channel].tx_config) {
        mtx_unlock(&mtx);
        return NULL;
    }

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    // Lock resources
    driver_unit_lock_error_t *lock_error = NULL;

    if ((lock_error = driver_lock(RMT_DRIVER, channel, GPIO_DRIVER, pin, DRIVER_ALL_FLAGS, NULL))) {
        mtx_unlock(&mtx);

        return driver_lock_error(RMT_DRIVER, lock_error);
    }
#endif

    // Configure RMT, TX part
    rmt_config_t rmt_tx;
    esp_err_t ret;

    rmt_tx.channel = channel;
    rmt_tx.gpio_num = pin;
    rmt_tx.rmt_mode = RMT_MODE_TX;

    // Set divider
    if (range == RMTPulseRangeNSEC) {
        // Count in nanoseconds, but with APB_CLK_FREQ resolution of RMT is 12.5 nanoseconds
        rmt_tx.clk_div = 1;
        devices[channel].tx.scale = 12.5;
    } else if (range == RMTPulseRangeUSEC) {
        // Count in microseconds
        rmt_tx.clk_div = (uint8_t)(APB_CLK_FREQ / 1000000UL);
        devices[channel].tx.scale = 1;
    } else if (range == RMTPulseRangeMSEC) {
        // Count in milliseconds
        rmt_tx.clk_div = (uint8_t)(APB_CLK_FREQ / 1000UL);
        devices[channel].tx.scale = 1;
    }

    rmt_tx.mem_block_num = 1;
    rmt_tx.tx_config.loop_en = 0;
    rmt_tx.tx_config.carrier_en = 0;

    if (idle_level == RMTIdleZ) {
        rmt_tx.tx_config.idle_output_en = 0;
        rmt_tx.tx_config.idle_level = 0;
    } else if (idle_level == RMTIdleL) {
        rmt_tx.tx_config.idle_output_en = 1;
        rmt_tx.tx_config.idle_level = 0;
    } else if (idle_level == RMTIdleH) {
        rmt_tx.tx_config.idle_output_en = 1;
        rmt_tx.tx_config.idle_level = 1;
    }

    assert(rmt_config(&rmt_tx) == ESP_OK);

    // Be sure that driver is not installed for channel, but first time esp-idf shows an error.
    // Unfortunately esp-idf hasn't an API to check if driver installed, so we avoid esp-idf to
    // show the error.
    esp_log_level_set("rmt", ESP_LOG_NONE);
    rmt_driver_uninstall(channel);
    esp_log_level_set("rmt", CONFIG_LOG_DEFAULT_LEVEL);

    if ((ret = rmt_driver_install(channel, 0, ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED)) != ESP_OK) {
        if (ret == ESP_ERR_NO_MEM) {
            return driver_error(RMT_DRIVER, RMT_ERR_NOT_ENOUGH_MEMORY, NULL);
        } else {
            assert(ret != ESP_ERR_INVALID_STATE);
        }
    }

    rmt_register_tx_end_callback(tx_end, NULL);

    // Create mutex for channel, if not yet created
    if (!mtx_inited(&devices[channel].mtx)) {
        mtx_init(&devices[channel].mtx, NULL, NULL, 0);
    }

    devices[channel].pin = pin;
    devices[channel].tx.range = range;
    *deviceid = channel;

    mtx_unlock(&mtx);

    return NULL;
}

driver_error_t *rmt_rx(int deviceid, rmt_item_t *rx, size_t rx_pulses, uint32_t timeout) {
    uint8_t channel = deviceid; // RMT channel
    rmt_item_t *cbuff;          // Current position in rx buffer

    // Convert timeout to FreeRTOS ticks
    if (devices[channel].rx.range == RMTPulseRangeNSEC) {
        timeout = ceil(((double)timeout / 1000000.0) / portTICK_PERIOD_MS);
    } else if (devices[channel].rx.range == RMTPulseRangeUSEC) {
        timeout = ceil(((double)timeout / 1000.0) / portTICK_PERIOD_MS);
    } else if (devices[channel].rx.range == RMTPulseRangeMSEC) {
        timeout = ceil((double)timeout / portTICK_PERIOD_MS);
    }

    mtx_lock(&devices[channel].mtx);

    assert(rmt_set_pin(channel, RMT_MODE_RX, devices[channel].pin) == ESP_OK);
    assert(rmt_rx_start(channel, 1) == ESP_OK);

    rmt_item32_t *ritems;         // Items received in current iteration
    uint32_t pending = rx_pulses; // Number of pending pulses
    size_t items = 0;              // Number of items received in current iteration

    cbuff = rx;
    while (pending > 0) {
        // Wait for data
        ritems = (rmt_item32_t*)xRingbufferReceive(devices[channel].rb, &items, timeout);
        if (ritems) {
            // Process only as much items as pending pulses
            items = ((items <= pending)?items:pending);

            // Copy to reception buffer
            memcpy(cbuff, ritems, items * sizeof(rmt_item32_t));
            cbuff += items;

            pending -= items;

            // Return items to ring buffer
            vRingbufferReturnItem(devices[channel].rb, (void *)ritems);
        } else {
            // No data received, timeout
            mtx_unlock(&devices[channel].mtx);

            return driver_error(RMT_DRIVER, RMT_ERR_TIMEOUT, NULL);
        }
    }

    assert(rmt_rx_stop(channel) == ESP_OK);

    mtx_unlock(&devices[channel].mtx);

    // RX buffer must be expressed in channel's range time units, so scale values if it's required
    cbuff = rx;

    if (devices[channel].rx.scale != 1.0) {
        int i;

        for(i = 0; i < rx_pulses;i++) {
            cbuff->duration0 *= devices[channel].tx.scale;
            cbuff->duration1 *= devices[channel].tx.scale;

            cbuff++;
        }
    }

    return NULL;
}

driver_error_t *rmt_tx(int deviceid, rmt_item_t *tx, size_t tx_pulses) {
    uint8_t channel = deviceid; // RMT channel
    rmt_item_t *cbuff;          // Current position in tx buffer

    mtx_lock(&devices[channel].mtx);

    // TX buffer is expressed in channel's range time units, so scale values if it's required
    if (devices[channel].tx.scale != 1.0) {
        int i;

        cbuff = tx;
        for(i = 0; i < tx_pulses;i++) {
            cbuff->duration0 /= devices[channel].tx.scale;
            cbuff->duration1 /= devices[channel].tx.scale;

            cbuff++;
        }
    }

    // Start RMT and transmit data
    rmt_set_pin(channel, RMT_MODE_TX, devices[channel].pin);
    assert(rmt_write_items(channel, (rmt_item32_t *)tx, tx_pulses, 1) == ESP_OK);
    assert(rmt_tx_stop(channel) == ESP_OK);

    mtx_unlock(&devices[channel].mtx);

    return NULL;
}

driver_error_t *rmt_tx_rx(int deviceid, rmt_item_t *tx, size_t tx_pulses, rmt_item_t *rx, size_t rx_pulses, uint32_t timeout) {
    uint8_t channel = deviceid; // RMT channel
    rmt_item_t *cbuff;          // Current position in tx / rx buffer

    mtx_lock(&devices[channel].mtx);

    // TX buffer is expressed in channel's range time units, so scale values if it's required
    if (devices[channel].tx.scale != 1.0) {
        int i;

        cbuff = tx;
        for(i = 0; i < tx_pulses;i++) {
            cbuff->duration0 /= devices[channel].tx.scale;
            cbuff->duration1 /= devices[channel].tx.scale;

            cbuff++;
        }
    }

    // When transmission is ended, RMT is configured in reception mode, and reception is started
    // as soon as possible. This is done installing a transmission end callback, which is executed
    // inside the RMT ISR.
    devices[channel].tx.callback = switch_rx;

    // Start RMT and transmit data
    rmt_set_pin(channel, RMT_MODE_TX, devices[channel].pin);

    assert(rmt_write_items(channel, (rmt_item32_t *)tx, tx_pulses, 1) == ESP_OK);

    // At this point reception was started in the transmission end callback, wait for
    // data reception
    devices[channel].tx.callback = NULL;

    // Convert timeout to FreeRTOS ticks
    if (devices[channel].rx.range == RMTPulseRangeNSEC) {
        timeout = ceil(((double)timeout / 1000000.0) / portTICK_PERIOD_MS);
    } else if (devices[channel].rx.range == RMTPulseRangeUSEC) {
        timeout = ceil(((double)timeout / 1000.0) / portTICK_PERIOD_MS);
    } else if (devices[channel].rx.range == RMTPulseRangeMSEC) {
        timeout = ceil((double)timeout / portTICK_PERIOD_MS);
    }

    rmt_item32_t *ritems;         // Items received in current iteration
    uint32_t pending = rx_pulses; // Number of pending pulses
    size_t items = 0;             // Number of items received in current iteration

    cbuff = rx;

    int retries = 0;
    while (pending > 0) {
        // Wait for data
        ritems = (rmt_item32_t*)xRingbufferReceive(devices[channel].rb, &items, timeout);
        if (ritems) {
            // Process only as much items as pending pulses
            items = ((items <= pending)?items:pending);

            // Copy to reception buffer
            memcpy(cbuff, ritems, items * sizeof(rmt_item32_t));
            cbuff += items;

            pending -= items;

            // Return items to ring buffer
            vRingbufferReturnItem(devices[channel].rb, (void *)ritems);
        } else {
            // No data received
            if (retries < 10) {
                retries++;
                continue;
            }

            assert(rmt_rx_stop(channel) == ESP_OK);

            mtx_unlock(&devices[channel].mtx);
            return driver_error(RMT_DRIVER, RMT_ERR_TIMEOUT, NULL);
        }
    }

    assert(rmt_rx_stop(channel) == ESP_OK);

    mtx_unlock(&devices[channel].mtx);

    // RX buffer must be expressed in channel's range time units, so scale values if it's required
    cbuff = rx;

    if (devices[channel].rx.scale != 1.0) {
        int i;

        for(i = 0; i < rx_pulses;i++) {
            cbuff->duration0 *= devices[channel].tx.scale;
            cbuff->duration1 *= devices[channel].tx.scale;

            cbuff++;
        }
    }

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

        // Destroy device mtx
        mtx_destroy(&devices[channel].mtx);

        // Uninstall channel
        rmt_driver_uninstall(channel);
    }

    mtx_unlock(&mtx);

    return NULL;
}

#endif

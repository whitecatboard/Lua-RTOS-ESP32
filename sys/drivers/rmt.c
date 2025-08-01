/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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

#include "rmt.h"
#include "cpu.h"
#include "gpio.h"
#include "driver.h"
#include "mutex.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "rmt_private.h"

#include "esp_rom_gpio.h"
#include "soc/rmt_periph.h"
#include "soc/rtc.h"
#include "hal/rmt_ll.h"
#include "hal/gpio_hal.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static void rmt_init();
static struct mtx mtx;

// Register driver and messages
DRIVER_REGISTER_BEGIN(RMT,rmt,CPU_LAST_RMT_CH - CPU_FIRST_RMT_CH + 1,rmt_init,NULL);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidPulseRange, "invalid pulse range", RMT_ERR_INVALID_PULSE_RANGE);
    DRIVER_REGISTER_ERROR(RMT, rmt, NotEnoughtMemory, "not enough memory", RMT_ERR_NOT_ENOUGH_MEMORY);
    DRIVER_REGISTER_ERROR(RMT, rmt, NoMoreRMT, "no more channels available", RMT_ERR_NO_MORE_RMT);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidPin, "invalid pin", RMT_ERR_INVALID_PIN);
    DRIVER_REGISTER_ERROR(RMT, rmt, Timeout, "timeout", RMT_ERR_TIMEOUT);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidIdleLevel, "invalid idle level", RMT_ERR_INVALID_IDLE_LEVEL);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidTimeout, "invalid timeout", RMT_ERR_INVALID_TIMEOUT);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidFilterTicks, "invalid filter ticks", RMT_ERR_INVALID_FILTER_TICKS);
    DRIVER_REGISTER_ERROR(RMT, rmt, InvalidIdleThreshold, "invalid idle threshold", RMT_ERR_INVALID_IDLE_THRESHOLD);
    DRIVER_REGISTER_ERROR(RMT, rmt, NotSupported, "not supported", RMT_ERR_NOT_SUPPORTED);
    DRIVER_REGISTER_ERROR(RMT, rmt, Fail, "fail", RMT_ERR_FAIL);
DRIVER_REGISTER_END(RMT,rmt,CPU_LAST_RMT_CH - CPU_FIRST_RMT_CH + 1,rmt_init,NULL);

typedef struct {
	rmt_symbol_word_t *buffer;
	uint32_t buffer_size;
	rmt_receive_config_t *config;
} switch_rx_args_t;

static rmt_device_t *devices = NULL;
static gpio_hal_context_t gpio_hal = {0};

/*
 * Helper functions
 */
static void rmt_init() {
    mtx_init(&mtx, NULL, NULL, 0);

    gpio_hal.dev = GPIO_HAL_GET_HW(GPIO_PORT_0);
}

static bool tx_end(rmt_channel_handle_t channel, const rmt_tx_done_event_data_t *edata, void *args) {
	// Get channel
	uint8_t chan_id = channel->channel_id;

	// Call callbacks
    if (devices[chan_id].tx.callback) {
    	devices[chan_id].tx.callback(chan_id, devices[chan_id].tx.callback_args);
    }

	return pdFALSE;
}

static bool rx_done(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    QueueHandle_t q = (QueueHandle_t)user_data;

    xQueueSendFromISR(q, edata, &xHigherPriorityTaskWoken);

    return xHigherPriorityTaskWoken;
}

static void rmt_prepare_for_tx(uint8_t channel) {
	// Get pin
	uint8_t pin = devices[channel].pin;

	// Get internal channel id
	uint8_t channel_id = devices[channel].tx.tx_chan->channel_id;

	// Disable input / enable output
    gpio_hal_input_disable(&gpio_hal, pin);
    gpio_hal_output_enable(&gpio_hal, pin);

    // Route signals
    rmt_group_t *group = devices[channel].tx.tx_chan->group;
    int group_id = group->group_id;

    esp_rom_gpio_connect_out_signal(
    	pin,
        rmt_periph_signals.groups[group_id].channels[channel_id + RMT_TX_CHANNEL_OFFSET_IN_GROUP].tx_sig,
        false, false
    );

    gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
}

static void rmt_prepare_for_rx(uint8_t channel) {
	// Get pin
	uint8_t pin = devices[channel].pin;

	// Get internal channel id
	uint8_t channel_id = devices[channel].tx.tx_chan->channel_id;

	// Disable output / enable input
    gpio_hal_output_disable(&gpio_hal, pin);
    gpio_hal_input_enable(&gpio_hal, pin);

    // Route signals
    rmt_group_t *group = devices[channel].rx.rx_chan->group;
    int group_id = group->group_id;

    esp_rom_gpio_connect_in_signal(
    	pin,
		rmt_periph_signals.groups[group_id].channels[channel_id + RMT_RX_CHANNEL_OFFSET_IN_GROUP].rx_sig,
        false
	);

    gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
}

static void switch_rx(int channel, void *args) {
	switch_rx_args_t *fargs = (switch_rx_args_t *)args;

    rmt_prepare_for_rx(channel);
    rmt_receive(devices[channel].rx.rx_chan, fargs->buffer, fargs->buffer_size, fargs->config);
}

static void rmt_internal_unsetup_tx(int deviceid) {
	// Get channel
    uint8_t channel = deviceid;

    // Unsetup if configured as TX
	if (devices[channel].tx_config) {
		// Disable and delete channel and encoder
		if (devices[channel].tx.tx_chan) {
			rmt_disable(devices[channel].tx.tx_chan);
			rmt_del_channel(devices[channel].tx.tx_chan);
		}

		if (devices[channel].tx.tx_encoder) {
			rmt_del_encoder(devices[channel].tx.tx_encoder);
		}

		// Device now is not for TX
		devices[channel].tx.tx_chan = NULL;
		devices[channel].tx.tx_encoder = NULL;
		devices[channel].tx_config = 0;
	}
}

static void rmt_internal_unsetup_rx(int deviceid) {
	// Get channel
    uint8_t channel = deviceid;

    // Unsetup if configured as RX
	if (devices[channel].rx_config) {
		// Disable and delete channel
		if (devices[channel].rx.rx_chan) {
			rmt_disable(devices[channel].rx.rx_chan);
			rmt_del_channel(devices[channel].rx.rx_chan);
		}

		// Delete queue
		if (devices[channel].rx.q) {
			vQueueDelete(devices[channel].rx.q);
		}

		// Device now is not for RX
		devices[channel].rx.rx_chan = NULL;
		devices[channel].rx.q = NULL;
		devices[channel].rx_config = 0;
	}
}

static void rmt_internal_unsetup(int deviceid) {
	// Get channel
    uint8_t channel = deviceid;

	if (!devices[channel].tx_config && !devices[channel].rx_config) {
        // Free device
        devices[channel].pin = -1;

        // Destroy channel mtx
        mtx_destroy(&devices[channel].mtx);
	}
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

static int rmt_create_devices() {
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

static driver_error_t *rmt_check(esp_err_t err) {
	switch (err) {
		case ESP_ERR_NO_MEM: return driver_error(RMT_DRIVER, RMT_ERR_NOT_ENOUGH_MEMORY, NULL);
		case ESP_ERR_NOT_FOUND: return driver_error(RMT_DRIVER, RMT_ERR_NO_MORE_RMT, NULL);
		case ESP_ERR_NOT_SUPPORTED: return driver_error(RMT_DRIVER, RMT_ERR_NOT_SUPPORTED, NULL);
		case ESP_FAIL: return driver_error(RMT_DRIVER, RMT_ERR_FAIL, NULL);
	}

	return NULL;
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
    if (rmt_create_devices() < 0) {
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

    // Create queue for receive
    QueueHandle_t receive_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    if (!receive_queue) {
        mtx_unlock(&mtx);

        return driver_error(RMT_DRIVER, RMT_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    // Set speed
    uint32_t speed = APB_CLK_FREQ;

    if (range == RMTPulseRangeNSEC) {
        // Count in nanoseconds, but with APB_CLK_FREQ resolution of RMT is 12.5 nanoseconds
        speed = APB_CLK_FREQ;
        devices[channel].rx.scale = 12.5;
        devices[channel].rx.signal_range_min_ns = filter_ticks;
        devices[channel].rx.signal_range_max_ns = idle_threshold;
    } else if (range == RMTPulseRangeUSEC) {
        // Count in microseconds
        speed = 1000000;
        devices[channel].rx.scale = 1;
        devices[channel].rx.signal_range_min_ns = filter_ticks;
        devices[channel].rx.signal_range_max_ns = idle_threshold * 1000;
    } else if (range == RMTPulseRangeMSEC) {
        // Count in milliseconds
        speed = 1000;
        devices[channel].rx.scale = 1;
        devices[channel].rx.signal_range_min_ns = filter_ticks;
        devices[channel].rx.signal_range_max_ns = idle_threshold * 1000000;
    }

    // Create new RX channel
    esp_err_t err;
    driver_error_t *error;

    rmt_rx_channel_config_t rx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = speed,
        .mem_block_symbols = 64,
        .gpio_num = pin,
    };

    err = rmt_new_rx_channel(&rx_channel_cfg, &devices[channel].rx.rx_chan);
    if ((error = rmt_check(err))) {
    	return error;
    }

    err = rmt_enable(devices[channel].rx.rx_chan);
    if ((err != ESP_ERR_INVALID_STATE) && (error = rmt_check(err))) {
    	return error;
    }

    // Install callbacks
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rx_done,
    };

    err = rmt_rx_register_event_callbacks(devices[channel].rx.rx_chan, &cbs, receive_queue);
    if ((err != ESP_ERR_INVALID_STATE) && (error = rmt_check(err))) {
    	return error;
    }

    // Create mutex for channel, if not yet created
    if (!mtx_inited(&devices[channel].mtx)) {
        mtx_init(&devices[channel].mtx, NULL, NULL, 0);
    }

    devices[channel].pin = pin;
    devices[channel].rx.range = range;
    devices[channel].rx.q = receive_queue;

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

    // Find an existing channel attached to pin
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

    // Get speed and scale
    uint32_t speed = APB_CLK_FREQ;

    if (range == RMTPulseRangeNSEC) {
        // Count in nanoseconds, but with APB_CLK_FREQ resolution of RMT is 12.5 nanoseconds
        speed = APB_CLK_FREQ;
        devices[channel].tx.scale = 12.5;
    } else if (range == RMTPulseRangeUSEC) {
        // Count in microseconds
        speed = 1000000;
        devices[channel].tx.scale = 1;
    } else if (range == RMTPulseRangeMSEC) {
        // Count in milliseconds
        speed = 1000;
        devices[channel].tx.scale = 1;
    }

    // Create new TX channel
    esp_err_t err;
    driver_error_t *error;

    rmt_tx_channel_config_t tx_chan_config = {
		.clk_src = RMT_CLK_SRC_DEFAULT, // select clock source
		.gpio_num = pin,
		.mem_block_symbols = 64,
		.resolution_hz = speed,
		.trans_queue_depth = 1,
		.flags.io_od_mode = (idle_level == RMTIdleZ),
    };

    err = rmt_new_tx_channel(&tx_chan_config, &devices[channel].tx.tx_chan);
    if ((error = rmt_check(err))) {
    	return error;
    }

    err = rmt_enable(devices[channel].tx.tx_chan);
    if ((err != ESP_ERR_INVALID_STATE) && (error = rmt_check(err))) {
    	return error;
    }

    // Create encoder
    rmt_copy_encoder_config_t config;
    err = rmt_new_copy_encoder(&config, &devices[channel].tx.tx_encoder);
    if ((error = rmt_check(err))) {
    	return error;
    }

    // Install callbacks
    rmt_tx_event_callbacks_t cbs = {
        .on_trans_done = tx_end,
    };

    err = rmt_tx_register_event_callbacks(devices[channel].tx.tx_chan, &cbs, NULL);
    if ((err != ESP_ERR_INVALID_STATE) && (error = rmt_check(err))) {
    	return error;
    }

    // Create mutex for channel, if not yet created
    if (!mtx_inited(&devices[channel].mtx)) {
        mtx_init(&devices[channel].mtx, NULL, NULL, 0);
    }

    devices[channel].pin = pin;
    devices[channel].tx.range = range;
    devices[channel].tx.idle_level = idle_level;

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

    rmt_prepare_for_rx(channel);

    // Start receive
    rmt_symbol_word_t raw_symbols[64];

    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = devices[channel].rx.signal_range_min_ns,
        .signal_range_max_ns = devices[channel].rx.signal_range_max_ns,
    };

    rmt_receive(devices[channel].rx.rx_chan, raw_symbols, sizeof(raw_symbols), &receive_config);

    // Receive
    uint32_t pending = rx_pulses; // Number of pending pulses
    size_t items = 0;             // Number of items received in current iteration
    rmt_rx_done_event_data_t      rx_data;

    cbuff = rx;
    while (pending > 0) {
        // wait for data
        if (xQueueReceive(devices[channel].rx.q, &rx_data, timeout) == pdPASS) {
            // Process only as much items as pending pulses
            items = ((rx_data.num_symbols <= pending)?rx_data.num_symbols:pending);

            // Copy to reception buffer
            memcpy(cbuff, rx_data.received_symbols, items * sizeof(rmt_item_t));
            cbuff += items;

            pending -= items;

            // start receive again
            rmt_receive(devices[channel].rx.rx_chan, raw_symbols, sizeof(raw_symbols), &receive_config);
        } else {
            // No data received, timeout
            mtx_unlock(&devices[channel].mtx);

            return driver_error(RMT_DRIVER, RMT_ERR_TIMEOUT, NULL);
        }
    }

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

    // Transmit
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
		.flags.eot_level = devices[channel].tx.idle_level,
    };

    rmt_prepare_for_tx(channel);
	rmt_transmit(devices[channel].tx.tx_chan, devices[channel].tx.tx_encoder, tx, tx_pulses * sizeof(rmt_item_t), &tx_config);
	rmt_tx_wait_all_done(devices[channel].tx.tx_chan, -1);

	mtx_unlock(&devices[channel].mtx);

    return NULL;
}

driver_error_t *rmt_tx_rx(int deviceid, rmt_item_t *tx, size_t tx_pulses, rmt_item_t *rx, size_t rx_pulses, uint32_t timeout) {
    uint8_t channel = deviceid; // RMT channel
    rmt_item_t *cbuff;          // Current position in tx buffer
    rmt_symbol_word_t raw_symbols[64];

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

    // Convert timeout to FreeRTOS ticks
    if (devices[channel].rx.range == RMTPulseRangeNSEC) {
        timeout = ceil(((double)timeout / 1000000.0) / portTICK_PERIOD_MS);
    } else if (devices[channel].rx.range == RMTPulseRangeUSEC) {
        timeout = ceil(((double)timeout / 1000.0) / portTICK_PERIOD_MS);
    } else if (devices[channel].rx.range == RMTPulseRangeMSEC) {
        timeout = ceil((double)timeout / portTICK_PERIOD_MS);
    }

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
		.flags.eot_level = devices[channel].tx.idle_level,
    };

    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = devices[channel].rx.signal_range_min_ns,
        .signal_range_max_ns = devices[channel].rx.signal_range_max_ns,
    };

    mtx_lock(&devices[channel].mtx);

    // When transmission is ended, RMT is configured in reception mode, and reception is started
    // as soon as possible. This is done installing a transmission end callback, which is executed
    // inside the RMT ISR.
    switch_rx_args_t switch_args = {
        .buffer = raw_symbols,
		.buffer_size = sizeof(raw_symbols),
		.config = &receive_config,
    };

    devices[channel].tx.callback = switch_rx;
    devices[channel].tx.callback_args = &switch_args;

    // Transmit
    rmt_prepare_for_tx(channel);
	rmt_transmit(devices[channel].tx.tx_chan, devices[channel].tx.tx_encoder, tx, tx_pulses * sizeof(rmt_item_t), &tx_config);
	rmt_tx_wait_all_done(devices[channel].tx.tx_chan, -1);

    // At this point reception was started in the transmission end callback, wait for
    // data reception
    devices[channel].tx.callback = NULL;

    // Receive
    uint32_t pending = rx_pulses; // Number of pending pulses
    size_t items = 0;             // Number of items received in current iteration
    rmt_rx_done_event_data_t      rx_data;

    cbuff = rx;
    while (pending > 0) {
        // wait for data
        if (xQueueReceive(devices[channel].rx.q, &rx_data, timeout) == pdPASS) {
            // Process only as much items as pending pulses
            items = ((rx_data.num_symbols <= pending)?rx_data.num_symbols:pending);

            // Copy to reception buffer
            memcpy(cbuff, rx_data.received_symbols, items * sizeof(rmt_item_t));
            cbuff += items;

            pending -= items;

            if (pending > 0) {
				// start receive again
				rmt_receive(devices[channel].rx.rx_chan, raw_symbols, sizeof(raw_symbols), &receive_config);
            }
        } else {
            // No data received, timeout
            mtx_unlock(&devices[channel].mtx);

            return driver_error(RMT_DRIVER, RMT_ERR_TIMEOUT, NULL);
        }
    }

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
    mtx_lock(&mtx);

    // Unsetup TX part
	rmt_internal_unsetup_tx(deviceid);

	// Unsetup device
	rmt_internal_unsetup(deviceid);

    mtx_unlock(&mtx);
}

void rmt_unsetup_rx(int deviceid) {
    mtx_lock(&mtx);

    // Unsetup RX part
	rmt_internal_unsetup_rx(deviceid);

	// Unsetup device
	rmt_internal_unsetup(deviceid);

    mtx_unlock(&mtx);
}

#endif

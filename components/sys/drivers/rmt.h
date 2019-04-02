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

#ifndef _DRIVERS_RMT_H_
#define _DRIVERS_RMT_H_

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"

#include <sys/driver.h>

typedef int rmt_pulse_idle_t;
typedef int rmt_idle_threshold_t;
typedef int rmt_filter_ticks_thresh_t;

typedef void (*rmt_callback_t)(int);

typedef enum {
    RMTPulseRangeNSEC = 0,
    RMTPulseRangeUSEC = 1,
    RMTPulseRangeMSEC = 2,
    RMTPulseRangeMAX
} rmt_pulse_range_t;

typedef enum {
    RMTIdleL,
    RMTIdleH,
    RMTIdleZ,
    RMTIdleMAX
} rmt_idle_level;

typedef struct {
    union {
        struct {
            uint32_t duration0 :15;
            uint32_t level0 :1;
            uint32_t duration1 :15;
            uint32_t level1 :1;
        };
        uint32_t val;
    };
} rmt_item_t;

typedef struct {
    int8_t pin;
    struct mtx mtx;
    RingbufHandle_t rb;

    uint8_t rx_config;
    struct {
        rmt_pulse_range_t range;
        float scale;
    } rx;

    uint8_t tx_config;
    struct {
        rmt_pulse_range_t range;
        float scale;
        rmt_callback_t callback;
    } tx;
} rmt_device_t;

// RMT errors
#define RMT_ERR_INVALID_PULSE_RANGE             (DRIVER_EXCEPTION_BASE(RMT_DRIVER_ID) |  0)
#define RMT_ERR_NOT_ENOUGH_MEMORY               (DRIVER_EXCEPTION_BASE(RMT_DRIVER_ID) |  1)
#define RMT_ERR_NO_MORE_RMT                     (DRIVER_EXCEPTION_BASE(RMT_DRIVER_ID) |  2)
#define RMT_ERR_INVALID_PIN                     (DRIVER_EXCEPTION_BASE(RMT_DRIVER_ID) |  3)
#define RMT_ERR_TIMEOUT                         (DRIVER_EXCEPTION_BASE(RMT_DRIVER_ID) |  4)
#define RMT_ERR_INVALID_IDLE_LEVEL              (DRIVER_EXCEPTION_BASE(RMT_DRIVER_ID) |  5)
#define RMT_ERR_INVALID_TIMEOUT                 (DRIVER_EXCEPTION_BASE(RMT_DRIVER_ID) |  6)
#define RMT_ERR_INVALID_FILTER_TICKS            (DRIVER_EXCEPTION_BASE(RMT_DRIVER_ID) |  7)
#define RMT_ERR_INVALID_IDLE_THRESHOLD          (DRIVER_EXCEPTION_BASE(RMT_DRIVER_ID) |  8)

extern const int rmt_errors;
extern const int rmt_error_map;

/**
 * @brief Setup a RMT channel attached to a pin for receive data.
 *
 * @param pin GPIO number.
 *
 * @param range Time unit that will be used to get pulse duration. Can be either RMTPulseRangeNSEC (nanoseconds),
 *              RMTPulseRangeUSEC (milliseconds) or RMTPulseRangeMSEC (milliseconds).
 *
 * @min_pulse Min pulse duration time, expressed in range units. Pulses with a duration time less than this values
 *            will be ignored.
 *
 * @param deviceid A pointer to an integer that will be used to get the device id.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *          RMT_ERR_INVALID_PIN
 *          RMT_ERR_INVALID_PULSE_RANGE
 *          RMT_ERR_NOT_ENOUGH_MEMORY
 *          RMT_ERR_NO_MORE_RMT
 */
driver_error_t *rmt_setup_rx(int pin, rmt_pulse_range_t range, rmt_filter_ticks_thresh_t filter_ticks, rmt_idle_threshold_t idle_threshold, int *deviceid);

/**
 * @brief Setup a RMT channel attached to a pin for transmit data.
 *
 * @param pin GPIO number.
 *
 * @param range Time unit that will be used to set pulse duration. Can be either RMTPulseRangeNSEC (nanoseconds),
 *              RMTPulseRangeUSEC (milliseconds) or RMTPulseRangeMSEC (milliseconds).
 *
 * @param idle_level GPIO logical level when RMT goes to the idle state. Can be either RMTIdleL (Low), RMTIdleH
 *                   (High), or RMTIdleZ (High impedance).
 *
 * @param deviceid A pointer to an integer that will be used to get the device id.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *          RMT_ERR_INVALID_PIN
 *          RMT_ERR_INVALID_PULSE_RANGE
 *          RMT_ERR_INVALID_IDLE_LEVEL
 *          RMT_ERR_NOT_ENOUGH_MEMORY
 *          RMT_ERR_NO_MORE_RMT
 */
driver_error_t *rmt_setup_tx(int pin, rmt_pulse_range_t pulse_range, rmt_idle_level idle_level, rmt_callback_t callback, int *deviceid);

/**
 * @brief Unsetup a RMT device for transmit data, and free all resources.
 *
 * @param deviceid RMT device id.
 *
 */
void rmt_unsetup_tx(int deviceid);

/**
 * @brief Unsetup a RMT device for receive data, and free all resources.
 *
 * @param deviceid RMT device id.
 *
 */
void rmt_unsetup_rx(int deviceid);

/**
 * @brief Receive a number of pulses from the RMT device until they are received. This function is thread safe.
 *
 * @param deviceid RMT device id.
 *
 * @param rx A pointer to a buffer of rmt_item_t structure, in which the received data will be returned.
 *           In this buffer, all pulse duration data is expressed in the decvice's pulse_range units
 *           (nanoseconds, microseconds, or milliseconds).
 *
 * @param rx_pulses Number of pulses to receive.
 *
 * @param timeout A timeout, expressed in milliseconds, to wait for receive the pulses. If not all the pulses
 *                are received within this time the function returns with an error.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *          RMT_ERR_TIMEOUT
 */
driver_error_t *rmt_rx(int deviceid, rmt_item_t *rx, size_t rx_pulses, uint32_t timeout);

/**
 * @brief Transmit a number of pulses to the RMT device until they are transmitted. This function is thread safe.
 *
 * @param deviceid RMT device id.
 *
 * @param tx A pointer to a buffer of rmt_item_t structure, which contains the pulses to transmit.
 *           In this buffer, all pulse duration is expressed in the device's pulse_range units
 *           (nanoseconds, microseconds, or milliseconds).
 *
 * @param tx_pulses Number of pulses to transmit.
 *
 * @return
 *     - NULL success
 */
driver_error_t *rmt_tx(int deviceid, rmt_item_t *tx, size_t tx_pulses);

/**
 * @brief Transmit a number of pulses to the RMT device until they are transmitted, and then, receive a number of
 *        pulses from the RMT device until they are received. The switch between transmission and reception is done
 *        inside an ISR, so this function can be used when a fast transition between transmission and reception is
 *        required. This function is thread safe.
 *
 * @param deviceid RMT device id.
 *
 * @param tx A pointer to a buffer of rmt_item_t structure, which contains the pulses to transmit.
 *           In this buffer, all pulse duration is expressed in the device's pulse_range units
 *           (nanoseconds, microseconds, or milliseconds).
 *
 * @param tx_pulses Number of pulses to transmit.
 *
 * @param rx A pointer to a buffer of rmt_item_t structure, in which the received data will be returned.
 *           In this buffer, all pulse duration data is expressed in the decvice's pulse_range units
 *           (nanoseconds, microseconds, or milliseconds).
 *
 * @param rx_pulses Number of pulses to receive.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *          RMT_ERR_TIMEOUT
 */
driver_error_t *rmt_tx_rx(int deviceid, rmt_item_t *tx, size_t tx_pulses, rmt_item_t *rx, size_t rx_pulses, uint32_t timeout);


#endif /* _DRIVERS_RMT_H_ */

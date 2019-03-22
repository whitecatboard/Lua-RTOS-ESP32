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

#include <sys/driver.h>

typedef enum {
	RMTPulseRangeUSEC = 0,
	RMTPulseRangeMSEC = 1,
	RMTPulseRangeMAX
} rmt_pulse_range_t;

typedef enum {
	RMTStatusUnknow,
	RMTStatusRX,
	RMTStatusTX
} rmt_status_t;

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

// RMT errors
#define RMT_ERR_INVALID_PULSE_RANGE             (DRIVER_EXCEPTION_BASE(RMT_DRIVER_ID) |  0)
#define RMT_ERR_NOT_ENOUGH_MEMORY               (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  1)
#define RMT_ERR_NO_MORE_RMT                     (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  2)
#define RMT_ERR_INVALID_PIN                     (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  3)
#define RMT_ERR_TIMEOUT                         (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  4)
#define RMT_ERR_UNEXPECTED                      (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  5)

extern const int rmt_errors;
extern const int rmt_error_map;

driver_error_t *rmt_setup_rx(int pin, rmt_pulse_range_t range, int *deviceid);
driver_error_t *rmt_rx(int deviceid, uint32_t pulses, uint32_t timeout, rmt_item_t **buffer);

#endif /* _DRIVERS_RMT_H_ */

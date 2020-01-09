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
 * Lua RTOS, NZR driver
 *
 */

/*
 * This driver implements NZR data transfers over a GPIO.
 */

#ifndef NZR_H_
#define NZR_H_

#include <sys/driver.h>

typedef struct {
	struct {
		uint32_t t0h; //T0H in nanos
		uint32_t t0l; //T0L in nanos
		uint32_t t1h; //T1H in nanos
		uint32_t t1l; //T1L in nanos
	} n;

	struct {
		uint32_t t0h; //T0H in cycles
		uint32_t t0l; //T0L in cycles
		uint32_t t1h; //T1H in cycles
		uint32_t t1l; //T1L in cycles
	} c;

	uint32_t res; //res in nanos
} nzr_timing_t;

typedef struct {
	uint8_t gpio;
	int deviceid;
	nzr_timing_t timings;
} nzr_instance_t;

// NZR errors
#define NZR_ERR_NOT_ENOUGH_MEMORY           (DRIVER_EXCEPTION_BASE(NZR_DRIVER_ID) |  0)
#define NRZ_ERR_INVALID_UNIT                (DRIVER_EXCEPTION_BASE(NZR_DRIVER_ID) |  1)

driver_error_t *nzr_setup(nzr_timing_t *timing, uint8_t gpio, uint32_t *unit);
driver_error_t *nzr_send(uint32_t unit, uint8_t *data, uint32_t bits);
driver_error_t *nzr_unsetup(uint32_t unit);

#endif /* NZR_H_ */

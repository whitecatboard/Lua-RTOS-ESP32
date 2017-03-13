/*
 * Lua RTOS, NZR driver
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

/*
 * This driver implements NZR data transfers over a GPIO.
 */

#ifndef NZR_H_
#define NZR_H_

#include <sys/driver.h>

typedef struct {
	uint32_t t0h; //T0H in cycles
	uint32_t t0l; //T0L in cycles
	uint32_t t1h; //T1H in cycles
	uint32_t t1l; //T1L in cycles
	uint32_t res; //RES in cycles
} nzr_timing_t;

typedef struct {
	uint8_t gpio;
	nzr_timing_t timings;
} nzr_instance_t;

// NZR errors
#define NZR_ERR_NOT_ENOUGH_MEMORY           (DRIVER_EXCEPTION_BASE(NZR_DRIVER_ID) |  0)
#define NRZ_ERR_INVALID_UNIT                (DRIVER_EXCEPTION_BASE(NZR_DRIVER_ID) |  1)

driver_error_t *nzr_setup(nzr_timing_t *timing, uint8_t gpio, uint32_t *unit);
driver_error_t *nzr_send(uint32_t unit, uint8_t *data, uint32_t bits);

#endif /* NZR_H_ */

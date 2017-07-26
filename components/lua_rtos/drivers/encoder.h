/*
 * Lua RTOS, encoder driver
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#ifndef ENCODER_H
#define	ENCODER_H

#include <stdint.h>

#include <sys/driver.h>

typedef enum {
	EncoderMove,
	EncoderSwitch
} encoder_deferred_data_type_t;

typedef void (*encoder_callback_t)(int, uint32_t, uint8_t);

typedef struct {
	int8_t A;            		 ///< A pin
	int8_t B;            		 ///< B pin
	int8_t SW;            		 ///< SW pin
	uint8_t state;      		 ///< Current state's machine state
	int32_t counter;    		 ///< Current counter value
	encoder_callback_t callback; ///< Callback function
	int callback_id;             ///< Callback id
	uint8_t deferred;            ///< Deferred callback?
} encoder_h_t;

typedef struct {
	encoder_h_t *h;
	uint32_t value;
	encoder_deferred_data_type_t type;
} encoder_deferred_data_t;

// Encoder errors
#define ENCODER_ERR_NOT_ENOUGH_MEMORY           (DRIVER_EXCEPTION_BASE(ENCODER_DRIVER_ID) |  0)
#define ENCODER_ERR_INVALID_PIN                 (DRIVER_EXCEPTION_BASE(ENCODER_DRIVER_ID) |  1)

driver_error_t *encoder_setup(int8_t a, int8_t b, int8_t sw, encoder_h_t **h);
driver_error_t *encoder_unsetup(encoder_h_t *h);
driver_error_t *encoder_register_callback(encoder_h_t *h, encoder_callback_t callback, int id, uint8_t deferred);
driver_error_t *encoder_read(encoder_h_t *h, int32_t *val);
driver_error_t *encoder_write(encoder_h_t *h, int32_t val);

#endif	/* ENCODER_H */

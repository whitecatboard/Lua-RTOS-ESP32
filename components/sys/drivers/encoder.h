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
 * Lua RTOS, encoder driver
 *
 */

#ifndef ENCODER_H
#define	ENCODER_H

#include <stdint.h>

#include <sys/driver.h>

typedef void (*encoder_callback_t)(int, int8_t, uint32_t, uint8_t);

typedef struct {
	int8_t A;            		 ///< A pin
	int8_t B;            		 ///< B pin
	int8_t SW;            		 ///< SW pin
	uint8_t state;      		 ///< Current state's machine state
	int32_t counter;    		 ///< Current counter value
	uint8_t sw_latch;			 ///< Current switch value
	encoder_callback_t callback; ///< Callback function
	int callback_id;             ///< Callback id
	uint8_t deferred;            ///< Deferred callback?
} encoder_h_t;

typedef struct {
	encoder_h_t *h;
	int8_t dir;
	uint32_t counter;
	uint32_t button;
} encoder_deferred_data_t;

// Encoder errors
#define ENCODER_ERR_NOT_ENOUGH_MEMORY           (DRIVER_EXCEPTION_BASE(ENCODER_DRIVER_ID) |  0)
#define ENCODER_ERR_INVALID_PIN                 (DRIVER_EXCEPTION_BASE(ENCODER_DRIVER_ID) |  1)

extern const int encoder_errors;
extern const int encoder_error_map;

driver_error_t *encoder_setup(int8_t a, int8_t b, int8_t sw, encoder_h_t **h);
driver_error_t *encoder_unsetup(encoder_h_t *h);
driver_error_t *encoder_register_callback(encoder_h_t *h, encoder_callback_t callback, int id, uint8_t deferred);
driver_error_t *encoder_read(encoder_h_t *h, int32_t *val, uint8_t *sw);
driver_error_t *encoder_write(encoder_h_t *h, int32_t val);

#endif	/* ENCODER_H */

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
 * Lua RTOS, servo driver
 *
 */

#ifndef _SERVO_H_
#define _SERVO_H_

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SERVO

#include <sys/driver.h>

#define SERVO_MID 1444
#define SERVO_USEC_PER_DEG 10

// Pulse width for middle position
//#define SERVO_MID SERVO_MIN + SERVO_WIDTH

// Servo instance
typedef struct servo_instance {
	int8_t   pin;
	uint16_t value;
	int8_t   pwm_channel;
} servo_instance_t;

// Servo errors
#define SERVO_ERR_CANT_INIT                (DRIVER_EXCEPTION_BASE(SERVO_DRIVER_ID) |  0)
#define SERVO_ERR_NOT_ENOUGH_MEMORY		   (DRIVER_EXCEPTION_BASE(SERVO_DRIVER_ID) |  1)

extern const int servo_errors;
extern const int servo_error_map;

// Driver functions
driver_error_t *servo_setup(int8_t pin, servo_instance_t **instance);
driver_error_t *servo_write(servo_instance_t *instance, double value);

#endif

#endif /* _SERVO_H_ */

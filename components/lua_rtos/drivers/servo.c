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

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SERVO

#include <math.h>
#include <string.h>

#include <sys/list.h>
#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/servo.h>
#include <drivers/pwm.h>

// Register drivers and errors
DRIVER_REGISTER_BEGIN(SERVO,servo,NULL,NULL,NULL);
	DRIVER_REGISTER_ERROR(SERVO, servo, CannotSetup, "can't setup", SERVO_ERR_CANT_INIT);
	DRIVER_REGISTER_ERROR(SERVO, servo, NotEnoughtMemory, "not enough memory", SERVO_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_END(SERVO,servo,NULL,NULL,NULL);

/*
 * Helper functions
 */
static double get_duty(servo_instance_t *instance) {
	return (double)instance->value / ((double)20000);
}

static uint16_t angle_to_pulse(servo_instance_t *instance, double angle) {
	uint16_t pulse;

	if (angle <= 90) {
		pulse = SERVO_MID - (90 - angle) * SERVO_USEC_PER_DEG;
	} else {
		pulse = SERVO_MID + (angle - 90) * SERVO_USEC_PER_DEG;
	}

	return pulse;
}

/*
 * Operation functions
 */
driver_error_t *servo_setup(int8_t pin, servo_instance_t **instance) {
	driver_error_t *error;

	// Allocate space for a servo instance
	*instance = calloc(1,sizeof(servo_instance_t));
	if (!*instance) {
		return driver_error(SERVO_DRIVER, SERVO_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	(*instance)->pin = pin;
	(*instance)->value = SERVO_MID;

	// Configure PWM channel (channel is assigned by PWM driver).
	// Servo frequency is 50 hertzs.
	if ((error = pwm_setup(0, -1, pin, 50, get_duty(*instance), &(*instance)->pwm_channel))) {
		free(*instance);
		return error;
	}

	// Start PWM generation
	if ((error = pwm_start(0, (*instance)->pwm_channel))) {
		free(*instance);
		return error;
	}

	return NULL;
}

driver_error_t *servo_write(servo_instance_t *instance, double value) {
	driver_error_t *error;

	if ((value >= 0) && (value <= 180)) {
		instance->value = angle_to_pulse(instance, value);
	} else {
		instance->value = value;
	}

	if ((error = pwm_set_duty(0, instance->pwm_channel, get_duty(instance)))) {
    	return error;
	}

	return NULL;
}

#endif

/*
 * Lua RTOS, servo driver
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

#include "luartos.h"

#if USE_SERVO

#include <math.h>
#include <string.h>

#include <sys/list.h>
#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/servo.h>
#include <drivers/pwm.h>

// Driver message errors
DRIVER_REGISTER_ERROR(SERVO, servo, CannotSetup, "can't setup", SERVO_ERR_CANT_INIT);
DRIVER_REGISTER_ERROR(SERVO, servo, NotEnoughtMemory, "not enough memory", SERVO_ERR_NOT_ENOUGH_MEMORY);

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
		return driver_setup_error(SERVO_DRIVER, SERVO_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	(*instance)->pin = pin;
	(*instance)->value = SERVO_MID;

    // Setup PWM unit, 0 in our case
	if ((error = pwm_setup(0))) {
		free(*instance);
    	return error;
	}

	// Configure PWM channel (channel is assigned by PWM driver).
	// Servo frequency is 50 hertzs.
	if ((error = pwm_setup_channel(0, -1, pin, 50, get_duty(*instance), &(*instance)->pwm_channel))) {
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

DRIVER_REGISTER(SERVO,servo,NULL,NULL,NULL);

#endif

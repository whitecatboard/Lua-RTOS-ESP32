/*
 * Lua RTOS, servo driver
 *
 * Copyright (C) 2015 - 2016
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

#ifndef _SERVO_H_
#define _SERVO_H_

#include "luartos.h"

#if USE_SERVO

#include <sys/driver.h>

// Standard servo middle position, expressed in pulse width (usecs)
#define SERVO_MID_WIDTH 1500

// Servo instance
typedef struct servo_instance {
	int8_t pin;
	double offset;
	double width;
	double angle;
	int8_t pwm_channel;
} servo_instance_t;

// Servo errors
#define SERVO_ERR_CANT_INIT                (DRIVER_EXCEPTION_BASE(SERVO_DRIVER_ID) |  0)
#define SERVO_ERR_NOT_ENOUGH_MEMORY		   (DRIVER_EXCEPTION_BASE(SERVO_DRIVER_ID) |  1)
#define SERVO_ERR_INVALID_TYPE			   (DRIVER_EXCEPTION_BASE(SERVO_DRIVER_ID) |  2)
#define SERVO_ERR_INVALID_OPERATION		   (DRIVER_EXCEPTION_BASE(SERVO_DRIVER_ID) |  3)

// Driver functions
driver_error_t *servo_setup(int8_t pin, double offset, double width, servo_instance_t **instance);
driver_error_t *servo_write(servo_instance_t *instance, double angle);
driver_error_t *servo_set_offset(servo_instance_t *instance, double offset);
driver_error_t *servo_set_width(servo_instance_t *instance, double width);

#endif

#endif /* _SERVO_H_ */

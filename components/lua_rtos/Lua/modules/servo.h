/*
 * Lua RTOS, servo wrapper
 *
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
 * and fitness.  In no servo shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#ifndef LSERVO_H
#define	LSERVO_H

#include <stdint.h>

#include <drivers/servo.h>

/*
 * Servo fundamentals
 *
 * Each 20 ms an active-high digital pulse controls the position, so
 * signal has a frequency of 50 hz.
 *
 * The pulse width goes from 1 ms to 2 ms, and 1.5 ms is the center of
 * range.
 *
 */

// Servo types
#define SERVO_POSITIONAL 0
#define SERVO_CONTINUOUS 1

// This constants define the duty cycles needed for each servo direction:
// stop, clockwise, or counter clock wise
#define SERVO_STOP_DUTY 0.075
#define SERVO_CW_DUTY   0.06
#define SERVO_CCW_DUTY  0.08

// User data for this module
typedef struct {
	servo_instance_t *instance;
} servo_userdata;

#endif	/* LSERVO_H */

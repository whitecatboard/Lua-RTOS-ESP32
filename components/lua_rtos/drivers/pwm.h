/*
 * Lua RTOS, PWM driver
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

#ifndef PWM_H
#define	PWM_H

#include <stdint.h>

#include <sys/driver.h>

// Resources used by PWM
typedef struct {
	uint8_t pin;
	uint8_t timer;
} pwm_resources_t;

// PWM errors
#define PWM_ERR_CANT_INIT                (DRIVER_EXCEPTION_BASE(PWM_DRIVER_ID) |  0)
#define PWM_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(PWM_DRIVER_ID) |  1)
#define PWM_ERR_INVALID_CHANNEL          (DRIVER_EXCEPTION_BASE(PWM_DRIVER_ID) |  2)
#define PWM_ERR_INVALID_DUTY             (DRIVER_EXCEPTION_BASE(PWM_DRIVER_ID) |  3)

driver_error_t *pwm_setup(int8_t unit);
driver_error_t *pwm_setup_channel(int8_t unit, int8_t channel, int8_t pin, int32_t freq, double duty, int8_t *achannel);
driver_error_t *pwm_start(int8_t unit, int8_t channel);
driver_error_t *pwm_stop(int8_t unit, int8_t channel);
driver_error_t *pwm_set_duty(int8_t unit, int8_t channel, double duty);

#endif	/* PWM_H */


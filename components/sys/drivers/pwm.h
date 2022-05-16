/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, PWM driver
 *
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
#define PWM_ERR_INVALID_FREQUENCY		 (DRIVER_EXCEPTION_BASE(PWM_DRIVER_ID) |  4)

extern const int pwm_errors;
extern const int pwm_error_map;

driver_error_t *pwm_setup(int8_t unit, int8_t channel, int8_t pin, int32_t freq, double duty, int8_t *achannel);
driver_error_t *pwm_start(int8_t unit, int8_t channel);
driver_error_t *pwm_stop(int8_t unit, int8_t channel);
driver_error_t *pwm_set_freq(int8_t unit, int8_t channel, int32_t freq);
driver_error_t *pwm_set_duty(int8_t unit, int8_t channel, double duty);
driver_error_t *pwm_unsetup(int8_t unit, int8_t channel);

#endif	/* PWM_H */


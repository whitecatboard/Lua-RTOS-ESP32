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

#include "luartos.h"

#if USE_PWM

#include "driver/periph_ctrl.h"
#include "driver/ledc.h"

#include <math.h>
#include <stdlib.h>

#include <sys/syslog.h>
#include <sys/driver.h>

#include <drivers/cpu.h>
#include <drivers/pwm.h>
#include <drivers/gpio.h>

#include <drivers/pwm.h>

// Resolution to use
#define PWM_BITS 12
#define PWM_MAX_VAL 4095.0

// This macro gets a reference for this driver into drivers array
#define PWM_DRIVER driver_get_by_name("pwm")

// Driver locks
driver_unit_lock_t pwm_locks[CPU_LAST_PWM_CH + 1];

// Driver message errors
DRIVER_REGISTER_ERROR(PWM, pwm, CannotSetup, "can't setup", PWM_ERR_CANT_INIT);
DRIVER_REGISTER_ERROR(PWM, pwm, InvalidUnit, "invalid unit", PWM_ERR_INVALID_UNIT);
DRIVER_REGISTER_ERROR(PWM, pwm, InvalidChannel, "invalid channel", PWM_ERR_INVALID_CHANNEL);
DRIVER_REGISTER_ERROR(PWM, pwm, InvalidDuty, "invalid duty", PWM_ERR_INVALID_DUTY);

// PWM structures
struct pwm {
    int8_t timer;
    int8_t pin;
    int8_t setup;
};

struct pwm pwm[CPU_LAST_PWM + 1][CPU_LAST_PWM_CH + 1] = {
	{
		{0,-1,0},
		{0,-1,0},
		{1,-1,0},
		{1,-1,0},
		{2,-1,0},
		{2,-1,0},
		{3,-1,0},
		{3,-1,0},
		{0,-1,0},
		{0,-1,0},
		{1,-1,0},
		{1,-1,0},
		{2,-1,0},
		{2,-1,0},
		{3,-1,0},
		{3,-1,0},
	}
};


/*
 * Helper functions
 *
 */

// Gets timer related to pwm channel
static int8_t pwm_timer(int8_t unit, int8_t channel) {
	return pwm[unit][channel].timer;
}

static driver_error_t *pwm_check_unit(int8_t unit, int8_t setup) {
	if ((unit < CPU_FIRST_PWM) || (unit > CPU_LAST_PWM)) {
		if (setup) {
			return driver_setup_error(PWM_DRIVER, PWM_ERR_CANT_INIT, "invalid unit");
		} else {
			return driver_setup_error(PWM_DRIVER, PWM_ERR_INVALID_UNIT, NULL);
		}
	}

	return NULL;
}

static driver_error_t *pwm_check_channel(int8_t unit, int8_t channel, int8_t setup) {
	switch (unit) {
		case 0:
			if (!((1 << channel) & (CPU_PWM0_ALL)) && (channel != -1)) {
				if (setup) {
					return driver_setup_error(PWM_DRIVER, PWM_ERR_CANT_INIT, "invalid channel");
				} else {
					return driver_setup_error(PWM_DRIVER, PWM_ERR_INVALID_CHANNEL, NULL);
				}
			}
			break;
	}

	return NULL;
}

static driver_error_t *pwm_check_duty(double duty, int8_t setup) {
	if ((duty < 0) || (duty > 1)) {
		if (setup) {
			return driver_setup_error(PWM_DRIVER, PWM_ERR_CANT_INIT, "invalid duty");
		} else {
			return driver_setup_error(PWM_DRIVER, PWM_ERR_INVALID_DUTY, NULL);
		}
	}
	return NULL;
}

/*
 * Operation functions
 *
 */
// Lock resources needed by ADC
driver_error_t *pwm_lock_resources(int8_t unit, int8_t channel, void *resources) {
	pwm_resources_t tmp_pwm_resources;

	if (!resources) {
		resources = &tmp_pwm_resources;
	}

	pwm_resources_t *pwm_resources = (pwm_resources_t *)resources;
    driver_unit_lock_error_t *lock_error = NULL;

    //adc_pins(channel, &adc_resources->pin);

    // Lock timer

    // Lock pin
    if ((lock_error = driver_lock(PWM_DRIVER, channel, GPIO_DRIVER, pwm_resources->pin))) {
    	// Revoked lock on pin
    	return driver_lock_error(PWM_DRIVER, lock_error);
    }

    return NULL;
}

driver_error_t *pwm_setup(int8_t unit) {
	driver_error_t *error = NULL;

	// Sanity checks
	if ((error = pwm_check_unit(unit, 1))) return error;

	// Enable module
	switch (unit) {
		case 0: periph_module_enable(PERIPH_LEDC_MODULE); break;
	}

	return NULL;
}

driver_error_t *pwm_setup_channel(int8_t unit, int8_t channel, int8_t pin, int32_t freq, double duty, int8_t *achannel) {
	driver_error_t *error = NULL;

	// Sanity checks
	if ((error = pwm_check_unit(unit, 1))) return error;
	if ((error = pwm_check_channel(unit, channel, 1))) return error;
	if ((error = pwm_check_duty(duty, 1))) return error;

	if (!(pin & GPIO_ALL)) {
		return driver_setup_error(PWM_DRIVER, PWM_ERR_CANT_INIT, "invalid pin");
	}

	// If channel is -1 means that channel assignement is made by driver
	if (channel == -1) {
		// Get a free channel
		int8_t cchannel;

		for(cchannel=CPU_FIRST_PWM_CH;cchannel<=CPU_LAST_PWM_CH;cchannel++) {
			if (!pwm[unit][cchannel].setup) {
				channel = cchannel;
				break;
			}
		}
	}

	if (achannel) {
		*achannel = channel;
	}

	// Lock resources
    pwm_resources_t resources;

    resources.pin = pin;
    resources.timer = pwm_timer(unit, channel);

    if ((error = pwm_lock_resources(unit, channel, &resources))) {
    	pwm[unit][channel].setup = 0;
		return error;
	}

	pwm[unit][channel].setup = 1;
    pwm[unit][channel].pin = pin;

    // Setup timer
    ledc_timer_config_t timer_conf = {
		.bit_num = PWM_BITS,
		.freq_hz = freq,
		.speed_mode = LEDC_HIGH_SPEED_MODE,
		.timer_num = resources.timer,
     };
     ledc_timer_config(&timer_conf);

     // Setup channel
     ledc_channel_config_t ledc_conf = {
		.channel = channel,
		.duty = (int32_t)(duty * (double)PWM_MAX_VAL),
		.gpio_num = resources.pin,
		.intr_type = LEDC_INTR_DISABLE,
		.speed_mode = LEDC_HIGH_SPEED_MODE,
		.timer_sel = resources.timer,
     };

     ledc_channel_config(&ledc_conf);
     ledc_timer_pause(LEDC_HIGH_SPEED_MODE, resources.timer);

   	 // Detach PWM pin
     PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pwm[unit][channel].pin], PIN_FUNC_GPIO);
     gpio_set_direction(pwm[unit][channel].pin, GPIO_MODE_OUTPUT);
     gpio_matrix_out(pwm[unit][channel].pin, 0x100, 0, 0);

 	 gpio_pin_clr(pwm[unit][channel].pin);

     return NULL;
}

driver_error_t *pwm_start(int8_t unit, int8_t channel) {
	driver_error_t *error = NULL;

	// Sanity checks
	if ((error = pwm_check_unit(unit, 0))) return error;
	if ((error = pwm_check_channel(unit, channel, 0))) return error;

	// Attach PWM pin
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pwm[unit][channel].pin], PIN_FUNC_GPIO);
    gpio_set_direction(pwm[unit][channel].pin, GPIO_MODE_OUTPUT);
    gpio_matrix_out(pwm[unit][channel].pin, LEDC_HS_SIG_OUT0_IDX + channel, 0, 0);

	// Start timer
	ledc_timer_resume(LEDC_HIGH_SPEED_MODE, pwm[unit][channel].timer);

	return NULL;
}

driver_error_t *pwm_stop(int8_t unit, int8_t channel) {
	driver_error_t *error = NULL;

	// Sanity checks
	if ((error = pwm_check_unit(unit, 0))) return error;
	if ((error = pwm_check_channel(unit, channel, 0))) return error;

	// Pause timer
	ledc_timer_pause(LEDC_HIGH_SPEED_MODE, pwm[unit][channel].timer);

	// Detach PWM pin
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pwm[unit][channel].pin], PIN_FUNC_GPIO);
    gpio_set_direction(pwm[unit][channel].pin, GPIO_MODE_OUTPUT);
    gpio_matrix_out(pwm[unit][channel].pin, 0x100, 0, 0);

	gpio_pin_clr(pwm[unit][channel].pin);

	return NULL;
}

// Set new duty cycle
driver_error_t *pwm_set_duty(int8_t unit, int8_t channel, double duty) {
	driver_error_t *error = NULL;

	// Sanity checks
	if ((error = pwm_check_unit(unit, 0))) return error;
	if ((error = pwm_check_channel(unit, channel, 0))) return error;
	if ((error = pwm_check_duty(duty, 0))) return error;

	// Duty is expressed in %, and we bust to convert to a value from
	// 0 and (2 ^ bits) - 1
	ledc_set_duty(LEDC_HIGH_SPEED_MODE, channel, (int32_t)(duty * (double)PWM_MAX_VAL));
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, channel);

	return NULL;
}

DRIVER_REGISTER(PWM,pwm,pwm_locks,NULL,NULL);

#endif

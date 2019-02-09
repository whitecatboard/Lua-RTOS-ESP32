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
 * Lua RTOS, PWM driver
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_PWM

#include "esp_log.h"

#include "driver/periph_ctrl.h"
#include "driver/ledc.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <sys/syslog.h>
#include <sys/driver.h>

#include <drivers/cpu.h>
#include <drivers/pwm.h>
#include <drivers/gpio.h>

#include <drivers/pwm.h>

// This macro gets a reference for this driver into drivers array
#define PWM_DRIVER driver_get_by_name("pwm")

// Register drivers and errors
DRIVER_REGISTER_BEGIN(PWM,pwm,CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS * (CPU_LAST_PWM_CH + 1),NULL,NULL);
	DRIVER_REGISTER_ERROR(PWM, pwm, CannotSetup, "can't setup", PWM_ERR_CANT_INIT);
	DRIVER_REGISTER_ERROR(PWM, pwm, InvalidUnit, "invalid unit", PWM_ERR_INVALID_UNIT);
	DRIVER_REGISTER_ERROR(PWM, pwm, InvalidChannel, "invalid channel", PWM_ERR_INVALID_CHANNEL);
	DRIVER_REGISTER_ERROR(PWM, pwm, InvalidDuty, "invalid duty", PWM_ERR_INVALID_DUTY);
	DRIVER_REGISTER_ERROR(PWM, pwm, InvalidFrequency, "invalid frequency", PWM_ERR_INVALID_FREQUENCY);
DRIVER_REGISTER_END(PWM,pwm,CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS * (CPU_LAST_PWM_CH + 1),NULL,NULL);

// PWM structures
struct pwm {
    int8_t timer;
    int8_t pin;
    int8_t setup;
    int8_t started;
    int8_t bits;
};

struct pwm pwm[CPU_LAST_PWM + 1][CPU_LAST_PWM_CH + 1] = {
	{
		{0,-1,0,0,15},
		{0,-1,0,0,15},
		{1,-1,0,0,15},
		{1,-1,0,0,15},
		{2,-1,0,0,15},
		{2,-1,0,0,15},
		{3,-1,0,0,15},
		{3,-1,0,0,15},
		{0,-1,0,0,15},
		{0,-1,0,0,15},
		{1,-1,0,0,15},
		{1,-1,0,0,15},
		{2,-1,0,0,15},
		{2,-1,0,0,15},
		{3,-1,0,0,15},
		{3,-1,0,0,15},
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
			return driver_error(PWM_DRIVER, PWM_ERR_CANT_INIT, "invalid unit");
		} else {
			return driver_error(PWM_DRIVER, PWM_ERR_INVALID_UNIT, NULL);
		}
	}

	return NULL;
}

static driver_error_t *pwm_check_channel(int8_t unit, int8_t channel, int8_t setup) {
	switch (unit) {
		case 0:
			if (!((1 << channel) & (CPU_PWM0_ALL)) && (channel != -1)) {
				if (setup) {
					return driver_error(PWM_DRIVER, PWM_ERR_CANT_INIT, "invalid channel");
				} else {
					return driver_error(PWM_DRIVER, PWM_ERR_INVALID_CHANNEL, NULL);
				}
			}
			break;
	}

	return NULL;
}

static driver_error_t *pwm_check_duty(double duty, int8_t setup) {
	if ((duty < 0) || (duty > 1)) {
		if (setup) {
			return driver_error(PWM_DRIVER, PWM_ERR_CANT_INIT, "invalid duty");
		} else {
			return driver_error(PWM_DRIVER, PWM_ERR_INVALID_DUTY, NULL);
		}
	}
	return NULL;
}

static driver_error_t *pwm_check_freq(int32_t freq, int8_t setup) {
	if ((freq <= 0)) {
		if (setup) {
			return driver_error(PWM_DRIVER, PWM_ERR_CANT_INIT, "invalid frequency");
		} else {
			return driver_error(PWM_DRIVER, PWM_ERR_INVALID_FREQUENCY, NULL);
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

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	pwm_resources_t tmp_pwm_resources;

	if (!resources) {
		resources = &tmp_pwm_resources;
	}

	pwm_resources_t *pwm_resources = (pwm_resources_t *)resources;

	driver_unit_lock_error_t *lock_error = NULL;

    //adc_pins(channel, &adc_resources->pin);

    // Lock timer

    // Lock pin
    if ((lock_error = driver_lock(PWM_DRIVER, channel, GPIO_DRIVER, pwm_resources->pin, DRIVER_ALL_FLAGS, NULL))) {
    	// Revoked lock on pin
    	return driver_lock_error(PWM_DRIVER, lock_error);
    }
#endif

    return NULL;
}

driver_error_t *pwm_setup(int8_t unit, int8_t channel, int8_t pin, int32_t freq, double duty, int8_t *achannel) {
	driver_error_t *error = NULL;

	// Sanity checks
	if ((error = pwm_check_unit(unit, 1))) return error;
	if ((error = pwm_check_channel(unit, channel, 1))) return error;
	if ((error = pwm_check_duty(duty, 1))) return error;
	if ((error = pwm_check_freq(freq, 1))) return error;

	if (!(pin & GPIO_ALL)) {
		return driver_error(PWM_DRIVER, PWM_ERR_CANT_INIT, "invalid pin");
	}

	// Enable module
	switch (unit) {
		case 0: periph_module_enable(PERIPH_LEDC_MODULE); break;
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
    pwm[unit][channel].started = 0;

    // Setup timer
    int bits;
    esp_err_t resp;

    esp_log_level_set("ledc", ESP_LOG_NONE);

    for(bits = 15;bits >= 10;bits--) {
        ledc_timer_config_t timer_conf = {
    		.duty_resolution = bits,
    		.freq_hz = freq,
    		.speed_mode = LEDC_HIGH_SPEED_MODE,
    		.timer_num = resources.timer,
         };

         resp = ledc_timer_config(&timer_conf);
         if (resp == ESP_OK) {
        	 break;
         }
    }

    esp_log_level_set("ledc", ESP_LOG_ERROR);

    if (resp != ESP_OK) {
		return driver_error(PWM_DRIVER, PWM_ERR_CANT_INIT, "invalid frequency");
    }

    pwm[unit][channel].bits = bits;

     // Setup channel
     ledc_channel_config_t ledc_conf = {
		.channel = channel,
		.duty = (int32_t)(duty * (double)(~(0xffffffff << pwm[unit][channel].bits))),
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

	pwm[unit][channel].started = 1;

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

	pwm[unit][channel].started = 0;

	return NULL;
}

// Set new frequency
driver_error_t *pwm_set_freq(int8_t unit, int8_t channel, int32_t freq) {
	driver_error_t *error = NULL;

	// Sanity checks
	if ((error = pwm_check_unit(unit, 0))) return error;
	if ((error = pwm_check_channel(unit, channel, 0))) return error;
	if ((error = pwm_check_freq(freq, 0))) return error;

	// Change frequency if needed
	if (ledc_get_freq(LEDC_HIGH_SPEED_MODE, pwm[unit][channel].timer) != freq) {
		if (pwm[unit][channel].started) {
			// Pause timer
			ledc_timer_pause(LEDC_HIGH_SPEED_MODE, pwm[unit][channel].timer);
		}

	    // Setup timer
	    int bits;
	    esp_err_t resp;

	    esp_log_level_set("ledc", ESP_LOG_NONE);

	    for(bits = 15;bits >= 10;bits--) {
	        ledc_timer_config_t timer_conf = {
	    		.duty_resolution = bits,
	    		.freq_hz = freq,
	    		.speed_mode = LEDC_HIGH_SPEED_MODE,
	    		.timer_num = pwm[unit][channel].timer,
	         };

	         resp = ledc_timer_config(&timer_conf);
	         if (resp == ESP_OK) {
	        	 break;
	         }
	    }

	    esp_log_level_set("ledc", ESP_LOG_ERROR);

	    if (resp != ESP_OK) {
			return driver_error(PWM_DRIVER, PWM_ERR_INVALID_FREQUENCY, NULL);
	    }

	    pwm[unit][channel].bits = bits;

		if (pwm[unit][channel].started) {
			// Start timer
			ledc_timer_resume(LEDC_HIGH_SPEED_MODE, pwm[unit][channel].timer);
		}
	}

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
	int32_t duty_val = (int32_t)(duty * (double)(~(0xffffffff << pwm[unit][channel].bits)));

	// Update duty if needed
	if (ledc_get_duty(LEDC_HIGH_SPEED_MODE, channel) != duty_val) {
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, channel, duty_val);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, channel);
	}

	return NULL;
}

driver_error_t *pwm_unsetup(int8_t unit, int8_t channel) {
	driver_error_t *error = NULL;

	// Sanity checks
	if ((error = pwm_check_unit(unit, 0))) return error;
	if ((error = pwm_check_channel(unit, channel, 0))) return error;

	// Stop PWM
	pwm_stop(unit, channel);

	// Clear data
	memset(&pwm[unit][channel], 0, sizeof(pwm));

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	// Unlock resources
	driver_unlock_all(PWM_DRIVER, channel);
#endif

	return NULL;
}

#endif

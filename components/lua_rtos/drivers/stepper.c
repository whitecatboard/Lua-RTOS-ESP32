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
 * Lua RTOS, stepper driver
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_STEPPER

#include "freertos/FreeRTOS.h"
#include "soc/timer_group_struct.h"
#include "driver/timer.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

#include <sys/delay.h>
#include <sys/list.h>
#include <sys/driver.h>
#include <sys/syslog.h>
#include <sys/mutex.h>

#include <drivers/stepper.h>
#include <drivers/gpio.h>

// Register driver and messages
static void stepper_init();

DRIVER_REGISTER_BEGIN(STEPPER,stepper,NULL,stepper_init,NULL);
	DRIVER_REGISTER_ERROR(STEPPER, stepper, NotEnoughtMemory, "not enough memory", STEPPER_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(STEPPER, stepper, InvalidUnit, "invalid unit", STEPPER_ERR_INVALID_UNIT);
	DRIVER_REGISTER_ERROR(STEPPER, stepper, NoMoreUnits, "no more units available", STEPPER_ERR_NO_MORE_UNITS);
	DRIVER_REGISTER_ERROR(STEPPER, stepper, UnitNotSetup, "unit is not setup", STEPPER_ERR_UNIT_NOT_SETUP);
	DRIVER_REGISTER_ERROR(STEPPER, stepper, InvalidPin, "invalid pin", STEPPER_ERR_INVALID_PIN);
	DRIVER_REGISTER_ERROR(STEPPER, stepper, InvalidDirection, "invalid direction", STEPPER_ERR_INVALID_DIRECTION);
DRIVER_REGISTER_END(STEPPER,stepper,NULL,stepper_init,NULL);

// Stepper units
static stepper_t stepper[NSTEP];

static struct mtx stepper_mutex;
timg_dev_t *stepper_timerg;        // Timer group
int stepper_timeri;                // Timer unit into timer group
static uint32_t start;             // Start stepper mask (1 = started, 0 = not started)

/*
 * Helper functions
 */
static void stepper_init() {
    mtx_init(&stepper_mutex, NULL, NULL, 0);
	memset(stepper,0,sizeof(stepper_t) * NSTEP);
}

static inline void stepper_update_frequency(int unit, double freq) {
    // Compute theoretical ticks, real ticks, and lost ticks per tick

    /*
     * We have 1 tick every (1 / STEPPER_HZ) seconds.
     *
     * We need (1 / freq) / (1 / STEPPER_HZ) ticks for generate a clock
     * pulse at freq Hz.
     *
     */
    double tticks = (((double)1 / freq) / ((double)1 / (double)STEPPER_HZ));

    /*
     * tticks must be round to the nearest integer less than tticks, because
     * we can't get exactly tticks.
     */
    double rticks = floor(tticks);

    /*
     * tticks - lticks is the number of ticks that we lost by every tick. This
     * value is accumulated in every tick interrupt, and when reaches a value
     * >= 1 we need to decrement ticks by 1 for compensate.
     */
    double lticks = tticks - rticks;

    /*
     * Store rticks / lticks, for later use.
     */
    stepper[unit].ticks = rticks;
    stepper[unit].lticks = lticks;
}

static void IRAM_ATTR stepper_isr(void *arg) {
    int timer_idx = (int) arg;
    uint32_t intr_status = stepper_timerg->int_st_timers.val;

    if((intr_status & BIT(timer_idx)) && timer_idx == stepper_timeri) {
    	uint32_t clock_mask = 0;  // Clock mask
        uint32_t dirs_mask  = 0;  // Direction (set) mask
        uint32_t dirc_mask  = 0;  // Direction (clear) mask
        uint32_t stop_mask  = 0;  // Stop mask

        uint8_t unit = 0;              // Current unit
        stepper_t *pstepper = stepper; // Current stepper (= stepper[unit])

        uint32_t started = start;      // Pending steppers

        while (started) {
            if (started & 0b00000001) {
            	// Increment ticks
                pstepper->cticks++;

                if (pstepper->cticks == pstepper->ticks) {
                	// Update lost ticks
                    pstepper->lost = pstepper->lost + pstepper->lticks;

                    pstepper->cticks = 0;
                    while (pstepper->lost >= 1.0) {
                    	// 1 ticks is lost

                    	// Increment ticks for compensate
                        pstepper->cticks++;

                        // Update lost
                        pstepper->lost = pstepper->lost - 1.0;
                    }

                    // Update clock mask
                    clock_mask |= (1 << pstepper->clock_pin);

                    // Update direction mask
                    if (pstepper->dir) {
                        dirs_mask |= (1 << pstepper->dir_pin);
                    } else {
                        dirc_mask |= (1 << pstepper->dir_pin);
                    }

                    // Stop condition
                    if (pstepper->steps == 1) {
                        start &= ~(1 << unit);

                        stop_mask |= (1 << unit);

                        continue;
                    }

                    // Ramp UP
                    if (pstepper->steps >= pstepper->steps_up) {
                        pstepper->current_freq += pstepper->freq_inc;
                        stepper_update_frequency(unit, pstepper->current_freq);
                    } else {
                        // Ramp DOWN
                        if (pstepper->steps <= pstepper->steps_down) {
                            pstepper->current_freq -= pstepper->freq_inc;
                            stepper_update_frequency(unit, pstepper->current_freq);
                        }
                    }

                    pstepper->steps--;
                }
            }

            unit++;
            pstepper++;
            started = started >> 1;
        }

        if (clock_mask) {
            // Update direction
    		GPIO.out_w1ts = dirs_mask;
    		GPIO.out_w1tc = dirc_mask;

            // Generate clock pulse
    		GPIO.out_w1ts = clock_mask;
    		udelay(STEPPER_CLOCK_PULSE);
    		GPIO.out_w1tc = clock_mask;
        }

        if (stop_mask) {
        	// STOP
        	mtx_unlock(&stepper_mutex);
        }

    	stepper_timerg->hw_timer[timer_idx].update = 1;
    	stepper_timerg->int_clr_timers.t1 = 1;
    	stepper_timerg->hw_timer[timer_idx].config.alarm_en = 1;
    }
}

static void stepper_setup_timer(int timer_group, int timer_idx) {
    stepper_timeri = timer_idx;

    if (timer_group == 0) {
    	stepper_timerg = &TIMERG0;
    } else {
    	stepper_timerg = &TIMERG1;
    }

    timer_config_t config;
    config.alarm_en = 1;
    config.auto_reload = 1;
    config.counter_dir = TIMER_COUNT_UP;
    config.divider = 1;
    config.intr_type = TIMER_INTR_LEVEL;
    config.counter_en = TIMER_PAUSE;

    /*Configure timer*/
    timer_init(timer_group, timer_idx, &config);
    /*Stop timer counter*/
    timer_pause(timer_group, timer_idx);
    /*Load counter value */
    timer_set_counter_value(timer_group, timer_idx, 0x00000000ULL);
    /*Set alarm value*/

    timer_set_alarm_value(timer_group, timer_idx,  ((TIMER_BASE_CLK / STEPPER_HZ) / 2) - STEPPER_TIMER_ADJ);

    /*Enable timer interrupt*/
    timer_enable_intr(timer_group, timer_idx);
    /*Set ISR handler*/
    timer_isr_register(timer_group, timer_idx, stepper_isr, (void*) timer_idx, ESP_INTR_FLAG_IRAM, NULL);
    /*Start timer counter*/
    timer_start(timer_group, timer_idx);
}

/*
 * Operation functions
 */
driver_error_t *stepper_setup(uint8_t step_pin, uint8_t dir_pin, uint8_t *unit) {
	driver_error_t *error;
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unit_lock_error_t *lock_error = NULL;
#endif
	int i;

	// Sanity checks
	if (step_pin > 31 ) {
		return driver_error(STEPPER_DRIVER, STEPPER_ERR_INVALID_PIN, "must be between 0 and 31");
	}

	if (dir_pin > 31 ) {
		return driver_error(STEPPER_DRIVER, STEPPER_ERR_INVALID_PIN, "must be between 0 and 31");
	}

	mtx_lock(&stepper_mutex);

	// Find a free unit
	for(i = 0; i < NSTEP; i++) {
		if (!stepper[i].setup) {
			*unit = i;
			break;
		}
	}

	if (i > NSTEP) {
		// No free unit
		mtx_unlock(&stepper_mutex);
		return driver_error(STEPPER_DRIVER, STEPPER_ERR_NO_MORE_UNITS, NULL);
	}

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	// Lock the step pin
    if ((lock_error = driver_lock(STEPPER_DRIVER, *unit, GPIO_DRIVER, step_pin, DRIVER_ALL_FLAGS, "STEP"))) {
    	// Revoked lock on pin
		mtx_unlock(&stepper_mutex);
    	return driver_lock_error(STEPPER_DRIVER, lock_error);
    }

	// Lock the dir pin
    if ((lock_error = driver_lock(STEPPER_DRIVER, *unit, GPIO_DRIVER, dir_pin, DRIVER_ALL_FLAGS, "DIR"))) {
    	// Revoked lock on pin
		mtx_unlock(&stepper_mutex);
    	return driver_lock_error(STEPPER_DRIVER, lock_error);
    }
#endif

    // Configure stepper pins, as output, initial low
    if ((error = gpio_pin_output(step_pin))) {
		mtx_unlock(&stepper_mutex);
    	return error;
    }

    if ((error = gpio_pin_output(dir_pin))) {
		mtx_unlock(&stepper_mutex);
    	return error;
    }

    gpio_ll_pin_clr(step_pin);
    gpio_ll_pin_clr(dir_pin);

	stepper[*unit].clock_pin = step_pin;
	stepper[*unit].dir_pin = dir_pin;
	stepper[*unit].setup = 1;

    stepper_setup_timer(TIMER_GROUP_0, TIMER_1);

	mtx_unlock(&stepper_mutex);

    syslog(LOG_INFO,"stepper%d, at pins step=%s%d, dir=%s%d", *unit,
           gpio_portname(step_pin),
		   gpio_name(step_pin),
           gpio_portname(dir_pin),
		   gpio_name(dir_pin));

    return NULL;
}

driver_error_t *stepper_move(uint8_t unit, uint8_t dir, uint32_t steps, uint32_t ramp, double ifreq, double efreq) {
	// Sanity checks
	if (dir > 1) {
		return driver_error(STEPPER_DRIVER, STEPPER_ERR_INVALID_DIRECTION, NULL);
	}
	if (unit > NSTEP) {
		// Invalid unit
		return driver_error(STEPPER_DRIVER, STEPPER_ERR_INVALID_UNIT, NULL);
	}

	mtx_lock(&stepper_mutex);

	if (!stepper[unit].setup) {
		// Unit not setup
		mtx_unlock(&stepper_mutex);
		return driver_error(STEPPER_DRIVER, STEPPER_ERR_UNIT_NOT_SETUP, NULL);
	}

	stepper[unit].steps = steps;

    stepper[unit].steps_up = steps - ramp + 1;
    stepper[unit].steps_down = ramp;

    stepper[unit].target_freq  = efreq;
    stepper[unit].current_freq = (ramp > 0?ifreq:efreq);

    if (ramp > 0) {
        stepper[unit].freq_inc = (stepper[unit].target_freq - ifreq) / ramp;
    } else {
        stepper[unit].freq_inc = 0;
    }

    stepper[unit].dir = dir;
    stepper[unit].lost = 0;
    stepper[unit].cticks = 0;

    stepper_update_frequency(unit, stepper[unit].current_freq);

	mtx_unlock(&stepper_mutex);

	return NULL;
}

void stepper_start(int mask) {
	mtx_lock(&stepper_mutex);

	portDISABLE_INTERRUPTS();
    start |= mask;
    portENABLE_INTERRUPTS();

    // This lock blocks the calling thread
    // Loc is released in the ISR when movement is done
	mtx_lock(&stepper_mutex);

	mtx_unlock(&stepper_mutex);
}

#endif

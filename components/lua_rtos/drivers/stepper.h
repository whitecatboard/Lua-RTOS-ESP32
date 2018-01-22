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

/*

 The stepper driver is controlled by an interrupt that triggers every 1 / STEPPER_HZ seconds.

 Clock signal is generated accessing the GPIO registers directly. PWM is not used because
 with PWM we cannot guarantee that a movement involving more than 1 stepper starts at the
 same time.

 For a stepper the movement is defined by:

 - Number of steps
 - Frequency (this defines the speed)

 For example:

    200 steps at 200 Hz = 1 step every 0.005 seconds
    200 steps at 100 Hz = 1 step every 0.01 seconds

 Due to that we have a resolution of 1 / STEPPER_HZ we can have an error when calculating the
 number of ticks we need to reach the stepper frequency. For example, if STEPPER_HZ = 1000000:

    base period = (1 / STEPPER_HZ) seconds = 0.00001 seconds
 	stepper frequency = 333 Hz
 	stepper period = (1 / 540) seconds = 0.00185 seconds
 	ticks = (stepper period) / (base period) = 185.185 ticks

 	We need 185.185 ticks for generate a clock pulse at 540 Hz, but we only can count 185 or 186
 	ticks. If we count 185 ticks we have an error of 0.185 ticks.

 The error is compensated in the interrupt handler. In the previous example every 6 ticks the
 interrupt handler increments 1 tick for compensate.

 */

#ifndef _STEPPER_H_
#define _STEPPER_H_

#include <math.h>

// Number of steppers
#define NSTEP 8

// Stepper base timer frequency
#define STEPPER_HZ 100000

// Stepper clock pulse in microseconds
#define STEPPER_CLOCK_PULSE 10

#define STEPPER_TIMER_ADJ 5

typedef struct {
	uint8_t  setup;         // Is this unit setup?
    uint8_t  clock_pin;     // Clock pin number
    uint8_t  dir_pin;       // Direction pin number

    uint32_t steps;         // Number of steps to do
    uint32_t steps_up;      // Number of ramp-up steps to do
    uint32_t steps_down;    // Number of ramp-down steps to do

    double target_freq;     // Target stepper clock frequency
    double current_freq;    // Current stepper clock frequency
    double freq_inc;        // Increment of current clock frequency

    uint32_t     ticks;     // Number of ticks to do for each clock pulse
    uint32_t     cticks;    // Number of ticks since last clock pulse
    double       lticks;    // Number of lost ticks per tick
    double       lost;      // Current lost ticks
    uint8_t      dir;       // Direction. 0 = ccw, 1 = cw
} stepper_t;

// Stepper errors
#define STEPPER_ERR_NOT_ENOUGH_MEMORY        (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  0)
#define STEPPER_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  1)
#define STEPPER_ERR_NO_MORE_UNITS            (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  2)
#define STEPPER_ERR_UNIT_NOT_SETUP           (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  3)
#define STEPPER_ERR_INVALID_PIN              (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  4)
#define STEPPER_ERR_INVALID_DIRECTION        (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  5)

extern const int stepper_errors;
extern const int stepper_error_map;

driver_error_t *stepper_setup(uint8_t step_pin, uint8_t dir_pin, uint8_t *unit);
driver_error_t *stepper_move(uint8_t unit, uint8_t dir, uint32_t steps, uint32_t ramp, double ifreq, double efreq);
void stepper_start(int mask);

#endif /* _STEPPER_H_ */

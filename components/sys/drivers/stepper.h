/*
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _STEPPER_H_
#define _STEPPER_H_

#include <math.h>

#include <driver/rmt.h>

#include <motion/motion.h>

// Max number of steppers
#define NSTEP 8

// Step pulse duration in nanos
#define STEPPER_PULSE_NANOS 3000.0F

// Nonos per RMT tick
#define STEPPER_RMT_NANOS_PER_TICK 25.0F

// Step pulse duration in RMT ticks
#define STEPPER_PULSE_TICKS (STEPPER_PULSE_NANOS / STEPPER_RMT_NANOS_PER_TICK)

#define STEPPER_RMT_BUFF_SIZE 64
#define STEPPER_RMT_HALF_BUFF_SIZE (STEPPER_RMT_BUFF_SIZE >> 1)

#define STEPPER_RMT_DATA_SIZE 640
#define STEPPER_RMT_MAX_DURATION (32767 >> 1)
#define STEPPER_STATS 0
#define STEPPER_DEBUG 0

typedef struct {
	uint8_t  setup;         // Is this stepper unit setup?
    uint8_t  step_pin;      // Step pin number
    uint8_t  dir_pin;       // Direction pin number
    float min_spd;          // Min speed
    float max_spd;          // Max speed
    float mac_acc;          // Max acceleration

    uint8_t  dir;           // Direction. 0 = ccw, 1 = cw
    uint32_t steps;         // Number of steps
    int32_t pos;            // position in steps
    float units;            // Displacement units

    float units_per_step;   // Units per step
    float steps_per_unit;   // Steps per unit

    uint32_t *rmt_data;           // Circular - buffer with precomputed RMT data
    uint32_t rmt_data_head;
    uint32_t rmt_data_tail;

    uint32_t *rmt_block;          // Pointer to RMT RAM block
    uint32_t *rmt_block_current;  // Current write position in RMT RAM block
    float    rmt_period;          // Current RMT period
    uint32_t rmt_ticks;           // Current RMT period in RMT ticks
    uint32_t rmt_ticks_remain;
    uint8_t  rmt_offset;          // When RMT has send the half of a block, points to the start
                                  // of the consumed half block. Can be 0 or 32.

    uint8_t  rmt_start;
    uint8_t  rmt_started;

    float current_time;
    float current_position;

    motion_t motion;
} stepper_t;

// Stepper errors
#define STEPPER_ERR_NOT_ENOUGH_MEMORY        (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  0)
#define STEPPER_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  1)
#define STEPPER_ERR_NO_MORE_UNITS            (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  2)
#define STEPPER_ERR_UNIT_NOT_SETUP           (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  3)
#define STEPPER_ERR_INVALID_PIN              (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  4)
#define STEPPER_ERR_INVALID_DIRECTION        (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  5)
#define STEPPER_ERR_INVALID_ACCELERATION     (DRIVER_EXCEPTION_BASE(STEPPER_DRIVER_ID) |  6)

extern const int stepper_errors;
extern const int stepper_error_map;

driver_error_t *stepper_setup(uint8_t step_pin, uint8_t dir_pin, float min_spd, float max_spd, float max_acc, float stpu, uint8_t *unit);
driver_error_t *stepper_move(uint8_t unit, float units, float initial_spd, float target_spd, float acc, float jerk);
driver_error_t *stepper_get_distance(uint8_t unit, float *units);
driver_error_t *stepper_set_position(uint8_t unit, float units);
driver_error_t *stepper_get_position(uint8_t unit, float *units);
driver_error_t *stepper_is_running(uint8_t unit, uint32_t* running);

void stepper_start(int mask, uint8_t async);
void stepper_stop(int mask, uint8_t async);

#endif /* _STEPPER_H_ */

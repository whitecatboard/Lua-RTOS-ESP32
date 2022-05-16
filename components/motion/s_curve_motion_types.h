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

#ifndef _MOTION_S_CURVE_MOTION_CONSTRAINTS_H_
#define _MOTION_S_CURVE_MOTION_CONSTRAINTS_H_

#include <stdint.h>

typedef struct {
    float v0; // Initial velocity
    float v;  // Target velocity
    float a;  // Max acceleration
    float j;  // Max jerk
    float s;  // Total displacement
    float t;  // Displacement time (if > 0.0)

    float units_per_step; // Units per step
    float steps_per_unit; // Steps per unit
} s_curve_motion_constraints;

typedef struct {
    // Current motion constraints
    float v0; // Initial velocity
    float v;  // Target velocity
    float a;  // Max acceleration
    float j;  // Max jerk
    float s;  // Total displacement
    float t;  // Displacement time (if > 0.0)

    uint32_t steps;       // Number of steps
    int32_t step;         // Current step
    float units_per_step; // Units per step
    float steps_per_unit; // Steps per unit
    float step_time;
    float current_time;
    float current_position;

    // Phase bounds
    struct {
        float s[8];           // Phase total displacement
        float v[8];           // Phase exit velocity
        float t[8];           // Phase displacement time
        float total_t;        // Total displacement time
        int32_t steps[8];     // Phase steps, in each step the displacement is
                              // increased in units_per_step units
        int32_t acc_steps[8];
    } bound;

    int8_t phase; // In which profile acceleration phase we are?

    // Motion profile data in current phase
    float j_; // Phase jerk
    float a_; // Phase entry acceleration
    float v_; // Phase entry velocity
    float s_; // Phase cumulative displacement
    float t_; // Phase cumulative time
} s_curve_motion_t;

#endif

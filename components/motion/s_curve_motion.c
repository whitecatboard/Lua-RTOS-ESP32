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

#include "motion.h"

#include "esp_attr.h"

#include <sys/syslog.h>

#include <math.h>
#include <stdio.h>

static void _compute_bounds(motion_t *pmotion) {
    float units = pmotion->s_curve.s;
    float v0 = pmotion->s_curve.v0;
    float v = pmotion->s_curve.v;
    float a = pmotion->s_curve.a;
    float j = pmotion->s_curve.j;

    float t1, t2, t3;

#if MOTION_DEBUG
    float t4, t5, t6, t7;
#endif

    pmotion->s_curve.v0 = v0;

    t1 = a / j; // (1.1)
    pmotion->s_curve.bound.v[1] = v0 + ((a * a) / (2.0 * j)); // (1.4)
    pmotion->s_curve.bound.s[1] = v0 * t1 + (j / 6.0) * t1 * t1 * t1; // (1.2)
    pmotion->s_curve.bound.steps[1] = floor(pmotion->s_curve.bound.s[1] * pmotion->s_curve.steps_per_unit);

    t2 = ((v - v0 - ((a * a) / j)) / a); // (2.1)
    pmotion->s_curve.bound.v[2] = v - ((a * a)/(2.0 * j)); // (3.2)
    pmotion->s_curve.bound.s[2] = pmotion->s_curve.bound.v[1] * t2 + 0.5 * a * t2 * t2; // (2.2)
    pmotion->s_curve.bound.steps[2] = floor(pmotion->s_curve.bound.s[2] * pmotion->s_curve.steps_per_unit);

    if ((t2 < 0.0) || (v < ((a * a) / (2.0 * j)))) { // (2.1),
        // phase 2 & 6 not possible
        t2 = 0;
        pmotion->s_curve.bound.v[2] = pmotion->s_curve.bound.v[1];
        pmotion->s_curve.bound.s[2] = 0;
        pmotion->s_curve.bound.steps[2] = 0;
    }

    t3 = a / j; // (3.1)
    pmotion->s_curve.bound.v[3] = v;
    pmotion->s_curve.bound.s[3] = pmotion->s_curve.bound.v[2] * t3 + 0.5 * a * t3 * t3 - (j / 6.0) * t3 * t3 * t3; // (3.3)
    pmotion->s_curve.bound.steps[3] = floor(pmotion->s_curve.bound.s[3] * pmotion->s_curve.steps_per_unit);

    // (4.4)
    if (units - 2.0 * (pmotion->s_curve.bound.s[1] + pmotion->s_curve.bound.s[2] + pmotion->s_curve.bound.s[3]) >= 0) {
        #if MOTION_DEBUG
        t4 = (units - 2.0 * (pmotion->s_curve.bound.s[1] + pmotion->s_curve.bound.s[2] + pmotion->s_curve.bound.s[3])) / v; // (4.3)
        #endif
        pmotion->s_curve.bound.v[4] = v;
        pmotion->s_curve.bound.s[4] = units - 2.0 * (pmotion->s_curve.bound.s[1] + pmotion->s_curve.bound.s[2] + pmotion->s_curve.bound.s[3]);
        pmotion->s_curve.bound.steps[4] = floor(pmotion->s_curve.bound.s[4] * pmotion->s_curve.steps_per_unit);
    } else {
        #if MOTION_DEBUG
        t4 = 0;
        #endif
        pmotion->s_curve.bound.v[4] = pmotion->s_curve.bound.v[3];
        pmotion->s_curve.bound.s[4] = 0;
        pmotion->s_curve.bound.steps[4] = 0;
    }

    #if MOTION_DEBUG
    t5 = t3; // (5.1)
    #endif
    pmotion->s_curve.bound.v[5] = pmotion->s_curve.bound.v[2]; // (5.3)
    pmotion->s_curve.bound.s[5] = pmotion->s_curve.bound.s[3]; // (5.2)
    pmotion->s_curve.bound.steps[5] = floor(pmotion->s_curve.bound.s[5] * pmotion->s_curve.steps_per_unit);

    #if MOTION_DEBUG
    t6 = t2; // (6.1)
    #endif
    pmotion->s_curve.bound.v[6] = pmotion->s_curve.bound.v[1]; // (6.3)
    pmotion->s_curve.bound.s[6] = pmotion->s_curve.bound.s[2]; // (6.2)
    pmotion->s_curve.bound.steps[6] = floor(pmotion->s_curve.bound.s[6] * pmotion->s_curve.steps_per_unit);

    #if MOTION_DEBUG
    t7 = t1; // (7.1)
    #endif
    pmotion->s_curve.bound.v[7] = v0; // (7.3)
    pmotion->s_curve.bound.s[7] = pmotion->s_curve.bound.s[1]; // (7.2)
    pmotion->s_curve.bound.steps[7] = floor(pmotion->s_curve.bound.s[7] * pmotion->s_curve.steps_per_unit);

    if (
            pmotion->s_curve.steps > pmotion->s_curve.bound.steps[1] + pmotion->s_curve.bound.steps[2] + pmotion->s_curve.bound.steps[3] +
                                     pmotion->s_curve.bound.steps[4] + pmotion->s_curve.bound.steps[5] + pmotion->s_curve.bound.steps[6] +
                                     pmotion->s_curve.bound.steps[7]
       ) {
        // Add remaining steps (due to errors) to phase 4 (travel)
        pmotion->s_curve.bound.steps[4] += pmotion->s_curve.steps -
                (
                        pmotion->s_curve.bound.steps[1] + pmotion->s_curve.bound.steps[2] + pmotion->s_curve.bound.steps[3] +
                        pmotion->s_curve.bound.steps[4] + pmotion->s_curve.bound.steps[5] + pmotion->s_curve.bound.steps[6] +
                        pmotion->s_curve.bound.steps[7]
                );
    }

    #if MOTION_DEBUG
    syslog(LOG_DEBUG, "  Total steps: %d",  pmotion->s_curve.steps);
    syslog(LOG_DEBUG, "  s-curve phases:");
    if (pmotion->s_curve.bound.steps[1]) syslog(LOG_DEBUG, "    t1 %9.4f s, v1 %9.4f units/s, s1 %9.4f units, %5d steps",  t1, pmotion->s_curve.bound.v[1], pmotion->s_curve.bound.s[1], pmotion->s_curve.bound.steps[1]);
    if (pmotion->s_curve.bound.steps[2]) syslog(LOG_DEBUG, "    t2 %9.4f s, v2 %9.4f units/s, s2 %9.4f units, %5d steps",  t2, pmotion->s_curve.bound.v[2], pmotion->s_curve.bound.s[2], pmotion->s_curve.bound.steps[2]);
    if (pmotion->s_curve.bound.steps[3]) syslog(LOG_DEBUG, "    t3 %9.4f s, v3 %9.4f units/s, s3 %9.4f units, %5d steps",  t3, pmotion->s_curve.bound.v[3], pmotion->s_curve.bound.s[3], pmotion->s_curve.bound.steps[3]);
    if (pmotion->s_curve.bound.steps[4]) syslog(LOG_DEBUG, "    t4 %9.4f s, v4 %9.4f units/s, s4 %9.4f units, %5d steps",  t4, pmotion->s_curve.bound.v[4], pmotion->s_curve.bound.s[4], pmotion->s_curve.bound.steps[4]);
    if (pmotion->s_curve.bound.steps[5]) syslog(LOG_DEBUG, "    t5 %9.4f s, v5 %9.4f units/s, s5 %9.4f units, %5d steps",  t5, pmotion->s_curve.bound.v[5], pmotion->s_curve.bound.s[5], pmotion->s_curve.bound.steps[5]);
    if (pmotion->s_curve.bound.steps[6]) syslog(LOG_DEBUG, "    t6 %9.4f s, v6 %9.4f units/s, s6 %9.4f units, %5d steps",  t6, pmotion->s_curve.bound.v[6], pmotion->s_curve.bound.s[6], pmotion->s_curve.bound.steps[6]);
    if (pmotion->s_curve.bound.steps[7]) syslog(LOG_DEBUG, "    t7 %9.4f s, v7 %9.4f units/s, s7 %9.4f units, %5d steps",  t7, pmotion->s_curve.bound.v[7], pmotion->s_curve.bound.s[7], pmotion->s_curve.bound.steps[7]);
    #endif
}

void s_curve_prepare(motion_t *pmotion) {
    float v0 = pmotion->s_curve.v0;
    float v = pmotion->s_curve.v;
    float a = pmotion->s_curve.a;
    float j = pmotion->s_curve.j;
    float units = pmotion->s_curve.s;

    float half_units = units / 2.0;
    float s_part = (a / j) * (2.0 * v0 + ((a*a)/j)); // (7.6)
    float s_full = ((v*v - v0*v0) / (2.0 * a)) + ((a * (v0 + v)) / (2 * j)); // (7.8)

    #if MOTION_DEBUG
    syslog(LOG_DEBUG, "  Full s-curve acceleration/deceleration displacement is %.4f units", 2.0 * s_full);
    syslog(LOG_DEBUG, "  Partial s-curve acceleration/deceleration displacement is %.4f units", 2.0 * s_part);
    #endif

    if (s_part > half_units) {
        float a_ = solve_third_order_newton(1.0 / (j*j), 0, (2.0 * v0) / j, -1.0 * half_units, a, 0.0001); // (7.9)
        float v_ = v0 + ((a_*a_)/j); // (7.14)

        if (v_ > v) {
            a_ = sqrt(j * (v - v0)); // (7.15)
            v_ = v0 + ((a*a)/j);     // (7.16)
        }

        a = a_;
        v = v_;

        #if MOTION_DEBUG
        syslog(LOG_DEBUG, "  Partial s-curve");
        syslog(LOG_DEBUG, "  Acceleration set to %.4f units/s^2", a);
        syslog(LOG_DEBUG, "  Velocity set to %.4f units/s", v);
        #endif
    } else if (s_full > half_units) {
        float v_ = solve_second_order_pos(1, (a*a)/j, ((a*a*v0)/j) - v0*v0 - units * a); // (7.19)

        if ((v_ == NAN) || (v_ <= 0.0) || (v_ > v)) {
            v_ = v0 + ((a*a)/j); // (7.21)
        }

        v = v_;

        #if MOTION_DEBUG
        syslog(LOG_DEBUG, "  Full s-curve");
        syslog(LOG_DEBUG, "  Acceleration set to %.4f units/s^2", a);
        syslog(LOG_DEBUG, "  Velocity set to %.4f units/s", v);
        #endif
    } else if (v < v0 + ((a*a)/j)) { // (7.24)
        a = sqrt(j*(v-v0));

        #if MOTION_DEBUG
        syslog(LOG_DEBUG, "  Full s-curve");
        syslog(LOG_DEBUG, "  Acceleration set to %.4f units/s^2", a);
        syslog(LOG_DEBUG, "  Velocity set to %.4f units/s", v);
        #endif
    }

    pmotion->s_curve.v = v;
    pmotion->s_curve.a = a;

    // Calculate the steps required to achieve the required displacement (units).
    // As we cannot perform fraction of steps, round it to the largest integer value less than or equal.
    pmotion->s_curve.steps = floor(units * pmotion->s_curve.steps_per_unit);

    // Set current step to 0
    pmotion->s_curve.step = 0;

    // Compute s-curve bounds
    _compute_bounds(pmotion);

    pmotion->s_curve.bound.steps[2] += pmotion->s_curve.bound.steps[1];
    pmotion->s_curve.bound.steps[3] += pmotion->s_curve.bound.steps[2];
    pmotion->s_curve.bound.steps[4] += pmotion->s_curve.bound.steps[3];
    pmotion->s_curve.bound.steps[5] += pmotion->s_curve.bound.steps[4];
    pmotion->s_curve.bound.steps[6] += pmotion->s_curve.bound.steps[5];
    pmotion->s_curve.bound.steps[7] += pmotion->s_curve.bound.steps[6];

    pmotion->s_curve.j = j;
    pmotion->s_curve.current_time = 0;
    pmotion->s_curve.current_position = 0;
    pmotion->s_curve.a = a;
    pmotion->s_curve.phase = 0;
}

float IRAM_ATTR s_curve_next(motion_t *pmotion) {
    #if MOTION_CURVE_DEBUG
    uint8_t new_phase = 0;
    #endif

    // Increment steps done
    pmotion->s_curve.step++;

    // Check in which profile phase we are, and compute entry velocity, entry acceleration, and
    // jerk
    if (pmotion->s_curve.step <= pmotion->s_curve.bound.steps[1]) {
        if (pmotion->s_curve.phase != 1) {
            pmotion->s_curve.a_ = 0;
            pmotion->s_curve.j_ = pmotion->s_curve.j;
            pmotion->s_curve.v_ = pmotion->s_curve.v0;
            pmotion->s_curve.s_ = 0;
            pmotion->s_curve.t_ = 0;

            #if MOTION_CURVE_DEBUG
            new_phase = 1;
            #endif
        }

        pmotion->s_curve.phase = 1;
    } else if (pmotion->s_curve.step <= pmotion->s_curve.bound.steps[2]) {
        if (pmotion->s_curve.phase != 2) {
            pmotion->s_curve.a_ = pmotion->s_curve.a;
            pmotion->s_curve.j_ = 0;
            pmotion->s_curve.v_ = pmotion->s_curve.bound.v[1];
            pmotion->s_curve.s_ = 0;
            pmotion->s_curve.t_ = 0;

            #if MOTION_CURVE_DEBUG
            new_phase = 1;
            #endif
        }

        pmotion->s_curve.phase = 2;
    } else if (pmotion->s_curve.step <= pmotion->s_curve.bound.steps[3]) {
        if (pmotion->s_curve.phase != 3) {
            pmotion->s_curve.a_ = pmotion->s_curve.a;
            pmotion->s_curve.j_ = -1.0 * pmotion->s_curve.j;
            pmotion->s_curve.v_ = pmotion->s_curve.bound.v[2];
            pmotion->s_curve.s_ = 0;
            pmotion->s_curve.t_ = 0;

            #if MOTION_CURVE_DEBUG
            new_phase = 1;
            #endif
        }

        pmotion->s_curve.phase = 3;
    } else if (pmotion->s_curve.step <= pmotion->s_curve.bound.steps[4]) {
        if (pmotion->s_curve.phase != 4) {
            pmotion->s_curve.a_ = 0;
            pmotion->s_curve.j_ = 0;
            pmotion->s_curve.v_ = pmotion->s_curve.bound.v[3];
            pmotion->s_curve.s_ = 0;
            pmotion->s_curve.t_ = 0;

            #if MOTION_CURVE_DEBUG
            new_phase = 1;
            #endif
        }

        pmotion->s_curve.phase = 4;
    } else if (pmotion->s_curve.step <= pmotion->s_curve.bound.steps[5]) {
        if (pmotion->s_curve.phase != 5) {
            pmotion->s_curve.a_ = 0;
            pmotion->s_curve.j_ = -1.0 * pmotion->s_curve.j;
            pmotion->s_curve.v_ = pmotion->s_curve.bound.v[4];
            pmotion->s_curve.s_ = 0;
            pmotion->s_curve.t_ = 0;

            #if MOTION_CURVE_DEBUG
            new_phase = 1;
            #endif
        }

        pmotion->s_curve.phase = 5;
    } else if (pmotion->s_curve.step <= pmotion->s_curve.bound.steps[6]) {
        if (pmotion->s_curve.phase != 6) {
            pmotion->s_curve.a_ = -1.0* pmotion->s_curve.a;
            pmotion->s_curve.j_ = 0;
            pmotion->s_curve.v_ = pmotion->s_curve.bound.v[5];
            pmotion->s_curve.s_ = 0;
            pmotion->s_curve.t_ = 0;

            #if MOTION_CURVE_DEBUG
            new_phase = 1;
            #endif
        }

        pmotion->s_curve.phase = 6;
    } else if (pmotion->s_curve.step <= pmotion->s_curve.bound.steps[7]) {
        if (pmotion->s_curve.phase != 7) {
            pmotion->s_curve.a_ = -1.0* pmotion->s_curve.a;
            pmotion->s_curve.j_ = pmotion->s_curve.j;
            pmotion->s_curve.v_ = pmotion->s_curve.bound.v[6];
            pmotion->s_curve.s_ = 0;
            pmotion->s_curve.t_ = 0;

            #if MOTION_CURVE_DEBUG
            new_phase = 1;
            #endif
        }

        pmotion->s_curve.phase = 7;
    }

    // Compute the elapse time between the current step and the next step
    float next_in;

    if (pmotion->s_curve.phase == 4) {
        next_in = pmotion->s_curve.step_time; // (8.14)
    } else if ((pmotion->s_curve.phase == 2) || (pmotion->s_curve.phase == 6)) {
        // We are in phase 2 or 6.
        float t = solve_second_order_pos(0.5 * pmotion->s_curve.a_, pmotion->s_curve.v_, -pmotion->s_curve.s_ - pmotion->s_curve.units_per_step); // (8.8)

        // Compute time to next step
        next_in = t - pmotion->s_curve.t_;

        pmotion->s_curve.t_ = t;
    } else {
        // We are in phase 1, 3, 5 or 7. We need to solve (8.2).
        float initial_guess;

        if (pmotion->s_curve.current_time == 0) {
            initial_guess = pmotion->s_curve.units_per_step / pmotion->s_curve.v_; // (8.6)
        } else {
            initial_guess = pmotion->s_curve.t_; // (8.7)
        }

        // Solve (8.1)
        float t = solve_third_order_newton(((pmotion->s_curve.j_) / 6.0), 0.5 * pmotion->s_curve.a_, pmotion->s_curve.v_, - pmotion->s_curve.s_ - pmotion->s_curve.units_per_step, initial_guess, 0.0001);

        // Compute time to next step
        next_in = t - pmotion->s_curve.t_;

        pmotion->s_curve.t_ = t;
    }

    // Increment stepper position into current phase
    pmotion->s_curve.s_ += pmotion->s_curve.units_per_step;

    // Update stepper's current position
    pmotion->s_curve.current_position += pmotion->s_curve.units_per_step;

    // Update stepper's current time
    pmotion->s_curve.current_time += next_in;

    pmotion->s_curve.step_time = next_in;

    #if MOTION_CURVE_DEBUG
    printf("%f %f %f %d\r\n", pmotion->s_curve.current_time, pmotion->s_curve.v_ + pmotion->s_curve.a_ * pmotion->s_curve.t_ + 0.5 * pmotion->s_curve.j_ * pmotion->s_curve.t_ * pmotion->s_curve.t_,new_phase?pmotion->s_curve.v_:0.0);
    #endif

    return next_in;
}

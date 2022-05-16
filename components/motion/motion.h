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

#ifndef _MOTION_MOTION_H_
#define _MOTION_MOTION_H_

#define MOTION_DEBUG 0
#define MOTION_CURVE_DEBUG 0

#include "motion_math.h"
#include "s_curve_motion_types.h"

struct motion;

typedef void (*motion_prepare_func_t)(struct motion *);
typedef float (*motion_next_func_t)(struct motion *);
typedef void (*motion_dump_func_t)(struct motion *);

typedef enum {
    MotionSCurve,
    MotionMax,
} motion_profile_t;

typedef struct {
    motion_profile_t accleration_profile;

    union {
        s_curve_motion_constraints s_curve;
    };
} motion_constraints_t;

typedef struct motion {
    motion_profile_t accleration_profile;

    motion_prepare_func_t _prepare;
    motion_next_func_t _next;

#if MOTION_DEBUG
    motion_dump_func_t _dump;
#endif

    union {
        s_curve_motion_t s_curve;
    };
} motion_t;

void motion_prepare(motion_constraints_t *pconstraints, motion_t *pmotion);
void motion_constraint_t(motion_t *pmotion, float t);
float motion_next(motion_t *pmotion);
void motion_dumnp(motion_t *pmotion);

#include "s_curve_motion.h"

#endif

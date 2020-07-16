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

#include <string.h>

void motion_prepare(motion_constraints_t *pconstraints, motion_t *pmotion) {
    if (pconstraints->accleration_profile == MotionSCurve) {
        pmotion->accleration_profile = MotionSCurve;

        memset(pmotion, 0, sizeof(motion_t));

        pmotion->s_curve.v0 = pconstraints->s_curve.v0;
        pmotion->s_curve.v  = pconstraints->s_curve.v;
        pmotion->s_curve.a  = pconstraints->s_curve.a;
        pmotion->s_curve.j  = pconstraints->s_curve.j;
        pmotion->s_curve.s  = pconstraints->s_curve.s;
        pmotion->s_curve.t  = pconstraints->s_curve.t;

        pmotion->s_curve.units_per_step = pconstraints->s_curve.units_per_step;
        pmotion->s_curve.steps_per_unit = pconstraints->s_curve.steps_per_unit;

        pmotion->_prepare = s_curve_prepare;
        pmotion->_next = s_curve_next;

		#if MOTION_DEBUG
        pmotion->_dump = s_curve_dump;
		#endif
    }

    pmotion->_prepare(pmotion);
}

void motion_constraint_t(motion_t *pmotion, float t) {
    if (pmotion->accleration_profile == MotionSCurve) {
        pmotion->s_curve.t  = t;
    }

    pmotion->_prepare(pmotion);
}

void motion_dumnp(motion_t *pmotion) {
	#if MOTION_DEBUG
	pmotion->_dump(pmotion);
	#endif
}

float IRAM_ATTR motion_next(motion_t *pmotion) {
    return pmotion->_next(pmotion);
}

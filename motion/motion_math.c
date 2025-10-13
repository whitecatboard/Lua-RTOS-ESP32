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

#include "luartos.h"

#include "motion_math.h"

#include <math.h>

#define FLT_MAX    1E+37

#include "esp_attr.h"

inline float __divsf3(float a, float b) {
    float inversed_b, tmp;
    __asm__ volatile (
        "recip0.s %0, %2\n"
        "const.s %1, 1\n"
        "msub.s %1, %2, %0\n"
        "madd.s %0, %0, %1\n"
        "const.s %1, 1\n"
        "msub.s %1, %2, %0\n"
        "maddn.s %0, %0, %1\n"
        :"=&f"(inversed_b),"=&f"(tmp):"f"(b)
    );
    return a * inversed_b;
}

float IRAM_ATTR solve_third_order_newton(float a, float b, float c, float d, float first_approximation, float err) {
    float previous_unknown;
    float error;
    float previous_error;
    float a3 = 3.0 * a;
    float b2 = 2.0 * b;

    // First iteration
    float unknown = first_approximation;
    float unknown_square = unknown * unknown;
    float unknown_third = unknown_square * unknown;

    unknown = unknown - (
            (a * unknown_third + b * unknown_square + c * unknown + d) /
            (a3 * unknown_square + b2 * unknown + c)
    );

    previous_unknown = unknown;

    // Second iteration
    unknown_square = unknown * unknown;
    unknown_third = unknown_square * unknown;

    unknown = unknown - (
            (a * unknown_third + b * unknown_square + c * unknown + d) /
            (a3 * unknown_square + b2 * unknown + c)
    );

    // Compute error
    error = fabs(unknown - previous_unknown);
    previous_error = FLT_MAX;

    // Iterate until current error <= err
    while ((error > err) && (error != previous_error)) {
        previous_unknown = unknown;
        
        unknown_square = unknown * unknown;
        unknown_third = unknown_square * unknown;

        unknown = unknown - (
                (a * unknown_third + b * unknown_square + c * unknown + d) /
                (a3 * unknown_square + b2 * unknown + c)
        );

        previous_error = error;
        error = fabs(unknown - previous_unknown);

        if (error > previous_error) {
            unknown = previous_unknown;
            break;
        }
    }

    return unknown;
}

float IRAM_ATTR solve_third_order_newton_fast(float a, float a3, float b, float b2, float c, float d, float first_approximation, float err, int32_t *iterations) {
    float previous_unknown;
    float error;
    float previous_error;
    uint32_t it = 0;

	float unknown;
	float unknown_square;
	float unknown_third;
	float f;
	float f_derivate;
	float h;

   	unknown = first_approximation;
   	
    do {
		it++;
		
    	unknown_square = unknown * unknown;
    	unknown_third = unknown_square * unknown;
		
		f = (a * unknown_third + b * unknown_square + c * unknown + d);
		f_derivate = (a3 * unknown_square + b2 * unknown + c);
		
		h = __divsf3(f,f_derivate);
		
		previous_unknown = unknown;
		unknown = previous_unknown - h;
		
	    // Compute error
	    error = fabs(unknown - previous_unknown);
	} while (error > err);
	
    *iterations = it;

    return unknown;
}

float IRAM_ATTR solve_second_order_pos(float a, float b, float c) {
    float discriminant;
    float unknown;

    discriminant = b*b - 4.0 * a * c;

    if (discriminant > 0.0) {
        unknown = (-1.0 * b + sqrt(discriminant)) / (2.0 * a);

        if (unknown < 0.0) {
            unknown = NAN;
        }
    } else if (discriminant == 0.0) {
        unknown = - (b / (2 * a));
        if (unknown < 0.0) {
            unknown = NAN;
        }
    } else {
        unknown = NAN;
    }

	if (unknown == NAN) {
		printf("oops in solve third order\r\n");		
	}
	
    return unknown;
}

float IRAM_ATTR solve_second_min_pos(float a, float b, float c) {
    float discriminant;
    float unknown;
    float unknown1;
    float unknown2;

    discriminant = b*b - 4.0 * a * c;

    if (discriminant > 0.0) {
        unknown1 = (-1.0 * b + sqrt(discriminant)) / (2.0 * a);
        unknown2 = (-1.0 * b - sqrt(discriminant)) / (2.0 * a);

        if (unknown1 < 0.0) {
            if (unknown2 < 0.0) {
                unknown = NAN;
            } else {
                unknown = unknown2;
            }
        } else {
            if (unknown2 < 0.0) {
                unknown = unknown1;
            } else {
                unknown = fmin(unknown1,unknown2);
            }
        }
    } else if (discriminant == 0.0) {
        unknown = - (b / (2 * a));
        if (unknown < 0.0) {
            unknown = NAN;
        }
    } else {
        unknown = NAN;
    }

	if (unknown == NAN) {
		printf("oops in solve second order\r\n");		
	}
	
    return unknown;
}

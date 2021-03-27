/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, numerical integration functions unit test
 *
 */

#include "unity.h"

#include <math/integral.h>

#define N 1001

static float f_1(float x) {
    return 1.0;
}

static float f_x(float x) {
    return x;
}

static float f_x2(float x) {
    return x * x;
}

static float f_x3(float x) {
    return x * x * x;
}

TEST_CASE("integral", "[math]") {
    integral_t *integral;
    float period = 1.0 / 400.0;
    float x = 0.0;
    float y = 0.0;
    float epsilon = 0.000001;
    int i = 0;

    // Simpson's rule
    //
    // Simpson's rule is exact for:
    //
    // f(x) = 1
    // f(x) = x
    // f(x) = x^2
    // f(x) = x^3

    // Integrate f(x) = 1 in the 0..(N-1)*period range
    //
    // The integral is (N-1)*period
    integral = integral_create(IntegralSimpson1_3, period);
    for(i = 0; i < N;i++) {
        x = i * period;
        y = integral_value(integral, f_1(x));
    }

    TEST_ASSERT(flt_near_eq(y, x, &epsilon));

    // Integrate f(x) = x in the 0..(N-1)*period range
    //
    // The integral is (((N-1)*period)^2) / 2
    integral_reset(integral);

    for(i = 0; i < N;i++) {
        x = i * period;
        y = integral_value(integral, f_x(x));
    }

    TEST_ASSERT(flt_near_eq(y,(x * x) / 2.0, &epsilon));

    // Integrate f(x) = x^2 in the 0..(N-1)*period range
    //
    // The integral is (((N-1)*period)^3) / 3
    integral_reset(integral);

    for(i = 0; i < N;i++) {
        x = i * period;
        y = integral_value(integral, f_x2(x));
    }

    TEST_ASSERT(flt_near_eq(y,(x * x * x) / 3.0, &epsilon));

    // Integrate f(x) = x^3 in the 0..(N-1)*period range
    //
    // The integral is (((N-1)*period)^4) / 4
    integral_reset(integral);

    for(i = 0; i < N;i++) {
        x = i * period;
        y = integral_value(integral, f_x3(x));
    }

    TEST_ASSERT(flt_near_eq(y,(x * x * x * x) / 4.0, &epsilon));

    integral_destroy(integral);
}

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
 * Lua RTOS, numerical integration functions, Simpson's 1/3 rule
 *
 */

#include "integral.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>

void integral_simpson1_3_reset(integral_t *integral) {
    integral->simpson1_3.values = 0;
    integral->simpson1_3.val = 0.0;
    integral->simpson1_3.acc_val = 0.0;
    integral->simpson1_3.sum = 0.0;
}

void integral_simpson1_3_create(integral_t *integral, float h) {
    integral->type = IntegralSimpson1_3;

    integral->simpson1_3.h = h;

    integral_simpson1_3_reset(integral);
}

void integral_simpson1_3_destroy(integral_t *integral) {
    free(integral);
}

float integral_simpson1_3_value(integral_t *integral, float value) {
    float result = NAN;

    if (integral->simpson1_3.values == 0) {
        integral->simpson1_3.x[0] = value;

        integral->simpson1_3.values++;
    } else if (integral->simpson1_3.values == 1) {
        integral->simpson1_3.x[1] = value;

        integral->simpson1_3.values++;
    } else if (integral->simpson1_3.values == 2) {
        integral->simpson1_3.x[2] = value;

        integral->simpson1_3.sum = (integral->simpson1_3.x[0] + 4.0 * integral->simpson1_3.x[1] + integral->simpson1_3.x[2]);
        integral->simpson1_3.val = (integral->simpson1_3.h / 3) * integral->simpson1_3.sum;
        result = integral->simpson1_3.val;

        integral->simpson1_3.values++;
    } else if (integral->simpson1_3.values == 3) {
        integral->simpson1_3.x[0] = integral->simpson1_3.x[1];
        integral->simpson1_3.x[1] = integral->simpson1_3.x[2];
        integral->simpson1_3.x[2] = value;

        result = integral->simpson1_3.val;

        integral->simpson1_3.values = 4;
    } else if (integral->simpson1_3.values == 4) {
        integral->simpson1_3.x[0] = integral->simpson1_3.x[1];
        integral->simpson1_3.x[1] = integral->simpson1_3.x[2];
        integral->simpson1_3.x[2] = value;

        float tmp_sum = (integral->simpson1_3.x[0] + 4.0 * integral->simpson1_3.x[1] + integral->simpson1_3.x[2]);
        float tmp_val = integral->simpson1_3.acc_val + (integral->simpson1_3.h / 3.0) * (integral->simpson1_3.sum + tmp_sum);

        if (__isinff(tmp_val)) {
            integral->simpson1_3.acc_val += integral->simpson1_3.sum * (integral->simpson1_3.h / 3.0);
            integral->simpson1_3.sum = tmp_sum;
        } else {
            integral->simpson1_3.sum += tmp_sum;
        }

        integral->simpson1_3.val = integral->simpson1_3.acc_val + (integral->simpson1_3.h / 3.0) * integral->simpson1_3.sum;
        result = integral->simpson1_3.val;

        integral->simpson1_3.values = 3;
    }

    return result;
}

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
 * Lua RTOS, soft filter library
 *
 */

#include "soft-filter.h"

#include <math.h>
#include <stdlib.h>
#include <stdarg.h>

/*
 * Helper functions
 */

static void filter_create_bw_low_pass(filter_t *filter, int order, float s, float f) {
    filter->butter_lp.n = order/2;

    filter->butter_lp.a =  (float *)calloc(filter->butter_lp.n, sizeof(float));
    filter->butter_lp.d1 = (float *)calloc(filter->butter_lp.n, sizeof(float));
    filter->butter_lp.d2 = (float *)calloc(filter->butter_lp.n, sizeof(float));
    filter->butter_lp.w0 = (float *)calloc(filter->butter_lp.n, sizeof(float));
    filter->butter_lp.w1 = (float *)calloc(filter->butter_lp.n, sizeof(float));
    filter->butter_lp.w2 = (float *)calloc(filter->butter_lp.n, sizeof(float));

    float a = tanf(M_PI * f/s);
    float a2 = a * a;
    float r;

    int i;

    for(i=0; i < filter->butter_lp.n; ++i){
        r = sinf(M_PI * (2.0 * i + 1.0)/(4.0 * filter->butter_lp.n));
        s = a2 + 2.0 * a * r + 1.0;
        filter->butter_lp.a[i] = a2/s;
        filter->butter_lp.d1[i] = 2.0 * (1-a2)/s;
        filter->butter_lp.d2[i] = -(a2 - 2.0 * a * r + 1.0)/s;
    }
}

static void filter_create_bw_high_pass(filter_t *filter, int order, float s, float f){
    filter->butter_hp.n =  order/2;

    filter->butter_hp.a =  (float *)calloc(filter->butter_hp.n, sizeof(float));
    filter->butter_hp.d1 = (float *)calloc(filter->butter_hp.n, sizeof(float));
    filter->butter_hp.d2 = (float *)calloc(filter->butter_hp.n, sizeof(float));
    filter->butter_hp.w0 = (float *)calloc(filter->butter_hp.n, sizeof(float));
    filter->butter_hp.w1 = (float *)calloc(filter->butter_hp.n, sizeof(float));
    filter->butter_hp.w2 = (float *)calloc(filter->butter_hp.n, sizeof(float));

    float a = tanf(M_PI * f/s);
    float a2 = a * a;
    float r;

    int i;

    for(i=0; i < filter->butter_hp.n; ++i){
        r = sinf(M_PI * (2.0 * ((float)i) + 1.0)/(4.0 * ((float)filter->butter_hp.n)));
        s = a2 + 2.0 * a * r + 1.0;
        filter->butter_hp.a[i] = 1.0/s;
        filter->butter_hp.d1[i] = 2.0 * (1-a2)/s;
        filter->butter_hp.d2[i] = -(a2 - 2.0 * a * r + 1.0)/s;
    }
}

static void filter_create_bw_band_pass(filter_t *filter, int order, float s, float fl, float fu){
    filter->butter_bp.n = order/4;

    filter->butter_bp.a =  (float *)calloc(filter->butter_bp.n, sizeof(float));
    filter->butter_bp.d1 = (float *)calloc(filter->butter_bp.n, sizeof(float));
    filter->butter_bp.d2 = (float *)calloc(filter->butter_bp.n, sizeof(float));
    filter->butter_bp.d3 = (float *)calloc(filter->butter_bp.n, sizeof(float));
    filter->butter_bp.d4 = (float *)calloc(filter->butter_bp.n, sizeof(float));

    filter->butter_bp.w0 = (float *)calloc(filter->butter_bp.n, sizeof(float));
    filter->butter_bp.w1 = (float *)calloc(filter->butter_bp.n, sizeof(float));
    filter->butter_bp.w2 = (float *)calloc(filter->butter_bp.n, sizeof(float));
    filter->butter_bp.w3 = (float *)calloc(filter->butter_bp.n, sizeof(float));
    filter->butter_bp.w4 = (float *)calloc(filter->butter_bp.n, sizeof(float));

    float a = cosf(M_PI * (fu+fl)/s)/cosf(M_PI * (fu-fl)/s);
    float a2 = a * a;
    float b = tanf(M_PI * (fu-fl)/s);
    float b2 = b*b;
    float r;
    int i;

    for(i=0; i<filter->butter_bp.n; ++i){
        r = sinf(M_PI * (2.0 * i+1.0)/(4.0 * filter->butter_bp.n));
        s = b2 + 2.0 * b * r + 1.0;
        filter->butter_bp.a[i]  = b2/s;
        filter->butter_bp.d1[i] = 4.0 * a * (1.0 + b * r)/s;
        filter->butter_bp.d2[i] = 2.0 * (b2 - 2.0 * a2 - 1.0)/s;
        filter->butter_bp.d3[i] = 4.0 * a * (1.0 - b * r)/s;
        filter->butter_bp.d4[i] = -(b2 - 2.0 * b * r + 1.0)/s;
    }
}

static void filter_create_bw_band_stop(filter_t *filter, int order, float s, float fl, float fu){
    filter->butter_bs.n = order/4;

    filter->butter_bs.a  = (float *)calloc(filter->butter_bs.n, sizeof(float));
    filter->butter_bs.d1 = (float *)calloc(filter->butter_bs.n, sizeof(float));
    filter->butter_bs.d2 = (float *)calloc(filter->butter_bs.n, sizeof(float));
    filter->butter_bs.d3 = (float *)calloc(filter->butter_bs.n, sizeof(float));
    filter->butter_bs.d4 = (float *)calloc(filter->butter_bs.n, sizeof(float));

    filter->butter_bs.w0 = (float *)calloc(filter->butter_bs.n, sizeof(float));
    filter->butter_bs.w1 = (float *)calloc(filter->butter_bs.n, sizeof(float));
    filter->butter_bs.w2 = (float *)calloc(filter->butter_bs.n, sizeof(float));
    filter->butter_bs.w3 = (float *)calloc(filter->butter_bs.n, sizeof(float));
    filter->butter_bs.w4 = (float *)calloc(filter->butter_bs.n, sizeof(float));

    float a = cosf(M_PI * (fu + fl)/s)/cosf(M_PI * (fu - fl)/s);
    float a2 = a * a;
    float b = tanf(M_PI * (fu - fl)/s);
    float b2 = b * b;
    float r;

    int i;
    for(i=0; i<filter->butter_bs.n; ++i){
        r = sinf(M_PI * (2.0 * i + 1.0)/(4.0 * filter->butter_bs.n));
        s = b2 + 2.0 * b * r + 1.0;
        filter->butter_bs.a[i]  = 1.0/s;
        filter->butter_bs.d1[i] = 4.0 * a * (1.0 + b * r)/s;
        filter->butter_bs.d2[i] = 2.0 * (b2 - 2.0 * a2 -1.0)/s;
        filter->butter_bs.d3[i] = 4.0 * a * (1.0 - b * r)/s;
        filter->butter_bs.d4[i] = -(b2 - 2.0 * b * r + 1.0)/s;
    }

    filter->butter_bs.r = 4.0 * a;
    filter->butter_bs.s = 4.0 * a2 + 2.0;
}

static void filter_free_bw_low_pass(filter_t* filter){
    free(filter->butter_lp.a);
    free(filter->butter_lp.d1);
    free(filter->butter_lp.d2);
    free(filter->butter_lp.w0);
    free(filter->butter_lp.w1);
    free(filter->butter_lp.w2);

    free(filter);
}

static void filter_free_bw_high_pass(filter_t* filter){
    free(filter->butter_hp.a);
    free(filter->butter_hp.d1);
    free(filter->butter_hp.d2);
    free(filter->butter_hp.w0);
    free(filter->butter_hp.w1);
    free(filter->butter_hp.w2);

    free(filter);
}

static void filter_free_bw_band_pass(filter_t* filter){
    free(filter->butter_bp.a);
    free(filter->butter_bp.d1);
    free(filter->butter_bp.d2);
    free(filter->butter_bp.d3);
    free(filter->butter_bp.d4);
    free(filter->butter_bp.w0);
    free(filter->butter_bp.w1);
    free(filter->butter_bp.w2);
    free(filter->butter_bp.w3);
    free(filter->butter_bp.w4);

    free(filter);
}

static void filter_free_bw_band_stop(filter_t* filter){
    free(filter->butter_bs.a);
    free(filter->butter_bs.d1);
    free(filter->butter_bs.d2);
    free(filter->butter_bs.d3);
    free(filter->butter_bs.d4);
    free(filter->butter_bs.w0);
    free(filter->butter_bs.w1);
    free(filter->butter_bs.w2);
    free(filter->butter_bs.w3);
    free(filter->butter_bs.w4);

    free(filter);
}

static float filter_bw_low_pass(filter_t *filter, float x){
    int i;

    for(i=0; i<filter->butter_lp.n; ++i){
        filter->butter_lp.w0[i] = filter->butter_lp.d1[i] * filter->butter_lp.w1[i] + filter->butter_lp.d2[i] * filter->butter_lp.w2[i] + x;
        x = filter->butter_lp.a[i] * (filter->butter_lp.w0[i] + 2.0 * filter->butter_lp.w1[i] + filter->butter_lp.w2[i]);
        filter->butter_lp.w2[i] = filter->butter_lp.w1[i];
        filter->butter_lp.w1[i] = filter->butter_lp.w0[i];
    }

    return x;
}

static float filter_bw_high_pass(filter_t *filter, float x){
    int i;

    for(i=0; i<filter->butter_hp.n; ++i){
        filter->butter_hp.w0[i] = filter->butter_hp.d1[i] * filter->butter_hp.w1[i] + filter->butter_hp.d2[i] * filter->butter_hp.w2[i] + x;
        x = filter->butter_hp.a[i] * (filter->butter_hp.w0[i] - 2.0 * filter->butter_hp.w1[i] + filter->butter_hp.w2[i]);
        filter->butter_hp.w2[i] = filter->butter_hp.w1[i];
        filter->butter_hp.w1[i] = filter->butter_hp.w0[i];
    }

    return x;
}

static float filter_bw_band_pass(filter_t *filter, float x){
    int i;

    for(i=0; i<filter->butter_bp.n; ++i){
        filter->butter_bp.w0[i] = filter->butter_bp.d1[i] * filter->butter_bp.w1[i] + filter->butter_bp.d2[i] * filter->butter_bp.w2[i]+ filter->butter_bp.d3[i] * filter->butter_bp.w3[i]+ filter->butter_bp.d4[i] * filter->butter_bp.w4[i] + x;
        x = filter->butter_bp.a[i] * (filter->butter_bp.w0[i] - 2.0 * filter->butter_bp.w2[i] + filter->butter_bp.w4[i]);
        filter->butter_bp.w4[i] = filter->butter_bp.w3[i];
        filter->butter_bp.w3[i] = filter->butter_bp.w2[i];
        filter->butter_bp.w2[i] = filter->butter_bp.w1[i];
        filter->butter_bp.w1[i] = filter->butter_bp.w0[i];
    }

    return x;
}

static float filter_bw_band_stop(filter_t *filter, float x){
    int i;

    for(i=0; i<filter->butter_bs.n; ++i){
        filter->butter_bs.w0[i] = filter->butter_bs.d1[i] * filter->butter_bs.w1[i] + filter->butter_bs.d2[i] * filter->butter_bs.w2[i]+ filter->butter_bs.d3[i] * filter->butter_bs.w3[i]+ filter->butter_bs.d4[i] * filter->butter_bs.w4[i] + x;
        x = filter->butter_bs.a[i] * (filter->butter_bs.w0[i] - filter->butter_bs.r * filter->butter_bs.w1[i] + filter->butter_bs.s * filter->butter_bs.w2[i]- filter->butter_bs.r * filter->butter_bs.w3[i] + filter->butter_bs.w4[i]);
        filter->butter_bs.w4[i] = filter->butter_bs.w3[i];
        filter->butter_bs.w3[i] = filter->butter_bs.w2[i];
        filter->butter_bs.w2[i] = filter->butter_bs.w1[i];
        filter->butter_bs.w1[i] = filter->butter_bs.w0[i];
    }

    return x;
}

/*
 * Operation functions
 */

filter_t *filter_create(filter_type_t type, ...) {
    // Sanity checks
    if (type >= FilterMax) {
        return NULL;
    }

    // Allocate space for filter
    filter_t *filter = calloc(1, sizeof(filter_t));
    if (filter == NULL) {
        return NULL;
    }

    filter->type = type;

    va_list param;

    va_start(param, type);

    switch (type) {
        case FilterButterLowPass: {
            int n;
            int s;
            int f;

            n = va_arg(param, int);
            s = (float)va_arg(param, double);
            f = (float)va_arg(param, double);

            filter_create_bw_low_pass(filter, n, s, f);
            break;
        }

        case FilterButterHighPass: {
            int n;
            int s;
            int f;

            n = va_arg(param, int);
            s = (float)va_arg(param, double);
            f = (float)va_arg(param, double);

            filter_create_bw_high_pass(filter, n, s, f);

            break;
        }

        case FilterButterBandPass: {
            int n;
            int s;
            int fl;
            int fu;

            n = va_arg(param, int);
            s = (float)va_arg(param, double);
            fl = (float)va_arg(param, double);
            fu = (float)va_arg(param, double);

            if (fu <= fl) {
                free(filter);
                va_end(param);

                return NULL;
            }

            filter_create_bw_band_pass(filter, n, s, fl, fu);

            break;
        }

        case FilterButterBandStop: {
            int n;
            int s;
            int fl;
            int fu;

            n = va_arg(param, int);
            s = (float)va_arg(param, double);
            fl = (float)va_arg(param, double);
            fu = (float)va_arg(param, double);

            if (fu <= fl) {
                free(filter);
                va_end(param);

                return NULL;
            }

            filter_create_bw_band_stop(filter, n, s, fl, fu);

            break;
        }

        default:
            break;
    }

    va_end(param);

    return filter;
}

void filter_destroy(filter_t *filter) {
    if (filter != NULL) {
        switch (filter->type) {
            case FilterButterLowPass: filter_free_bw_low_pass(filter);break;
            case FilterButterHighPass: filter_free_bw_high_pass(filter);break;
            case FilterButterBandPass: filter_free_bw_band_pass(filter);break;
            case FilterButterBandStop: filter_free_bw_band_stop(filter);break;

            default:
                break;
        }
    }
}

float filter_value(filter_t *filter, float x) {
    float filtered_x;

    filtered_x = 0.0;

    if (filter != NULL) {
        switch (filter->type) {
            case FilterButterLowPass: filtered_x = filter_bw_low_pass(filter, x);break;
            case FilterButterHighPass: filtered_x = filter_bw_high_pass(filter, x);break;
            case FilterButterBandPass: filtered_x = filter_bw_band_pass(filter, x);break;
            case FilterButterBandStop: filtered_x = filter_bw_band_stop(filter, x);break;

            default:
                break;
        }
    }

    return filtered_x;
}

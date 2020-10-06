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

#ifndef SOFT_FILTER_H_
#define SOFT_FILTER_H_

typedef enum {
    FilterButterLowPass,
    FilterButterHighPass,
    FilterButterBandPass,
    FilterButterBandStop,
    FilterMax
} filter_type_t;

typedef struct {
    filter_type_t type;

    union {
        struct {
            int n;
            float *a;
            float *d1;
            float *d2;
            float *w0;
            float *w1;
            float *w2;
        } butter_lp;

        struct {
            int n;
            float *a;
            float *d1;
            float *d2;
            float *w0;
            float *w1;
            float *w2;
        } butter_hp;

        struct {
            int n;
            float *a;
            float *d1;
            float *d2;
            float *d3;
            float *d4;
            float *w0;
            float *w1;
            float *w2;
            float *w3;
            float *w4;
        } butter_bp;

        struct {
            int n;
            float r;
            float s;
            float *a;
            float *d1;
            float *d2;
            float *d3;
            float *d4;
            float *w0;
            float *w1;
            float *w2;
            float *w3;
            float *w4;
        } butter_bs;
    };
} filter_t;

filter_t *filter_create(filter_type_t type, ...);
void filter_destroy(filter_t *filter);
float filter_value(filter_t *filter, float x);


#endif /* SOFT_FILTER_H_ */

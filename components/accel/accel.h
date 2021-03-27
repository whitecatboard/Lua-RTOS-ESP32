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
 * Lua RTOS, accelerometer common functions
 *
 */

/*
 * References:
 *
 * https://www.nxp.com/docs/en/application-note/AN3461.pdf
 *
 */

#ifndef SENSORS_ACCEL_H_
#define SENSORS_ACCEL_H_

#include <math/vector.h>
#include <math/integral.h>

#include <signnal/soft-filter.h>

#include "gesture_display.h"

/**
 * @brief Accelerometer data.
 */
typedef struct {
    uint8_t filter_stable;
    uint32_t meassure;

    float s;

    filter_t *x_g_hp_filter; /*!< g high pass filter for x axis */
    filter_t *y_g_hp_filter; /*!< g high pass filter for y axis */
    filter_t *z_g_hp_filter; /*!< g high pass filter for z axis */

    filter_t *x_g_lp_filter; /*!< g low pass filter for x axis */
    filter_t *y_g_lp_filter; /*!< g low pass filter for y axis */
    filter_t *z_g_lp_filter; /*!< g low pass filter for z axis */

    integral_t *x_v_integral; /*!< velocity integral for x axis */
    integral_t *y_v_integral; /*!< velocity integral for y axis */
    integral_t *z_v_integral; /*!< velocity integral for z axis */

    vector_t g; /*!< not filtered g vector */
    vector_t g_hp; /*!< high-pass filtered g vector */
    vector_t g_lp; /*!< low-pass filtered g vector */
    vector_t v; /*!< velocity vector, obtained from high-pass filter */
    float pitch; /* Pitch angle, obtained from low-pass filter */
    float roll; /* Roll angle, obtained from low-pass filter */

    accel_gesture_display_orient_t gest_display_orient;
} accel_t;

accel_t *accel_create(int n, float s, float f);
void accel_destroy(accel_t *accel);
void accel_reset(accel_t *accel);
void accel_process(accel_t *accel, vector_t *g);
void accel_get_g(accel_t *accel, vector_t *v);
void accel_get_g_hp(accel_t *accel, vector_t *v);
void accel_get_g_lp(accel_t *accel, vector_t *v);
void accel_get_v(accel_t *accel, vector_t *v);
float accel_get_pitch(accel_t *accel);
float accel_get_roll(accel_t *accel);
accel_gesture_display_orient_t accel_get_gest_display_orient(accel_t *accel);

#endif /* SENSORS_ACCEL_H_ */

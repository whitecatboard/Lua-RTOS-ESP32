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

#include "sdkconfig.h"
#include "accel.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

#define ACCEL_EARTH_G_ACCEL 9.800

/*
 * Helper functions
 */

static float _roll(vector_t *g) {
    float roll;

    if (g->z != 0.0) {
        roll = (atan2(-g->y, g->z) * 180.0) / M_PI;
    } else {
        roll = NAN;
    }

    return roll;
}

static float _pitch(vector_t *g) {
    float pitch;

    if ((g->y * g->y + g->z * g->z) != 0.0) {
        pitch = (atan2(g->x, sqrt(g->y * g->y + g->z * g->z)) * 180.0) / M_PI;
    } else {
        pitch = NAN;
    }

    return pitch;
}

/*
 * Operation functions
 */

accel_t *accel_create(int n, float s, float f) {
    accel_t *accel = calloc(1, sizeof(accel_t));
    if (accel == NULL) {
        return NULL;
    }

    accel->s = s;

    // Create high-pass filter
    accel->x_g_hp_filter = filter_create(FilterButterHighPass, n, s, f);
    if (accel->x_g_hp_filter == NULL) {
        accel_destroy(accel);
        return NULL;
    }

    accel->y_g_hp_filter = filter_create(FilterButterHighPass, n, s, f);
    if (accel->y_g_hp_filter == NULL) {
        accel_destroy(accel);
        return NULL;
    }

    accel->z_g_hp_filter = filter_create(FilterButterHighPass, n, s, f);
    if (accel->z_g_hp_filter == NULL) {
        accel_destroy(accel);
        return NULL;
    }

    // Create low-pass filter
    accel->x_g_lp_filter = filter_create(FilterButterLowPass, n, s, f);
    if (accel->x_g_lp_filter == NULL) {
        accel_destroy(accel);
        return NULL;
    }

    accel->y_g_lp_filter = filter_create(FilterButterLowPass, n, s, f);
    if (accel->y_g_lp_filter == NULL) {
        accel_destroy(accel);
        return NULL;
    }

    accel->z_g_lp_filter = filter_create(FilterButterLowPass, n, s, f);
    if (accel->z_g_lp_filter == NULL) {
        accel_destroy(accel);
        return NULL;
    }

    // Create integral to obtain velocity
    accel->x_v_integral = integral_create(IntegralSimpson1_3, 1.0 / s);
    if (accel->x_v_integral == NULL) {
        accel_destroy(accel);
        return NULL;
    }

    accel->y_v_integral = integral_create(IntegralSimpson1_3, 1.0 / s);
    if (accel->x_v_integral == NULL) {
        accel_destroy(accel);
        return NULL;
    }

    accel->z_v_integral = integral_create(IntegralSimpson1_3, 1.0 / s);
    if (accel->x_v_integral == NULL) {
        accel_destroy(accel);
        return NULL;
    }

    accel_reset(accel);

    return accel;
}

void accel_destroy(accel_t *accel) {
    if (accel == NULL) {
        return;
    }

    if (accel->x_g_hp_filter != NULL) free(accel->x_g_hp_filter);
    if (accel->y_g_hp_filter != NULL) free(accel->y_g_hp_filter);
    if (accel->z_g_hp_filter != NULL) free(accel->z_g_hp_filter);

    if (accel->x_g_lp_filter != NULL) free(accel->x_g_lp_filter);
    if (accel->y_g_lp_filter != NULL) free(accel->y_g_lp_filter);
    if (accel->z_g_lp_filter != NULL) free(accel->z_g_lp_filter);

    if (accel->x_v_integral != NULL) free(accel->x_v_integral);
    if (accel->y_v_integral != NULL) free(accel->y_v_integral);
    if (accel->z_v_integral != NULL) free(accel->z_v_integral);

    free(accel);
}

void accel_reset(accel_t *accel) {
    if (accel == NULL) {
        return;
    }

    accel->filter_stable = 0;
    accel->meassure = 0;

    filter_reset(accel->x_g_hp_filter);
    filter_reset(accel->y_g_hp_filter);
    filter_reset(accel->z_g_hp_filter);

    filter_reset(accel->x_g_lp_filter);
    filter_reset(accel->y_g_lp_filter);
    filter_reset(accel->z_g_lp_filter);

    integral_reset(accel->x_v_integral);
    integral_reset(accel->y_v_integral);
    integral_reset(accel->z_v_integral);

    vector_reset(&(accel->g));
    vector_reset(&(accel->g_hp));
    vector_reset(&(accel->g_lp));
    vector_reset(&(accel->v));

    accel->pitch = NAN;
    accel->roll = NAN;
    accel->gest_display_orient = AccelGestDisplayOrientUnknown;
}

void accel_process(accel_t *accel, vector_t *g) {
    if (accel == NULL) {
        return;
    }

    // Not filtered data
    accel->g.x = g->x;
    accel->g.y = g->y;
    accel->g.z = g->z;

    // High-pass filter data
    accel->g_hp.x = filter_value(accel->x_g_hp_filter, g->x);
    accel->g_hp.y = filter_value(accel->y_g_hp_filter, g->y);
    accel->g_hp.z = filter_value(accel->z_g_hp_filter, g->z);

    // Low-pass filter data
    accel->g_lp.x = filter_value(accel->x_g_lp_filter, g->x);
    accel->g_lp.y = filter_value(accel->y_g_lp_filter, g->y);
    accel->g_lp.z = filter_value(accel->z_g_lp_filter, g->z);

    if (!accel->filter_stable) {
        // Filter are not yet stable
        accel->meassure++;
        if (accel->meassure == accel->s) {
            accel->filter_stable = 1;
        }
    } else {
        // Filter stable, compute properties

        accel->pitch = _pitch(&(accel->g_lp));
        accel->roll = _roll(&(accel->g_lp));

        accel->v.x = integral_value(accel->x_v_integral, accel->g_hp.x * ACCEL_EARTH_G_ACCEL);
        accel->v.y = integral_value(accel->y_v_integral, accel->g_hp.y * ACCEL_EARTH_G_ACCEL);
        accel->v.z = integral_value(accel->z_v_integral, accel->g_hp.z * ACCEL_EARTH_G_ACCEL);

        accel->gest_display_orient = accel_gesture_display_orient(&(accel->g_lp));
    }
}

void accel_get_g(accel_t *accel, vector_t *v) {
    v->x = accel->g.x;
    v->y = accel->g.y;
    v->z = accel->g.z;
}

void accel_get_g_hp(accel_t *accel, vector_t *v) {
    v->x = accel->g_hp.x;
    v->y = accel->g_hp.y;
    v->z = accel->g_hp.z;
}

void accel_get_g_lp(accel_t *accel, vector_t *v) {
    v->x = accel->g_lp.x;
    v->y = accel->g_lp.y;
    v->z = accel->g_lp.z;
}

void accel_get_v(accel_t *accel, vector_t *v) {
    v->x = accel->v.x;
    v->y = accel->v.y;
    v->z = accel->v.z;
}

float accel_get_pitch(accel_t *accel) {
    if (accel->filter_stable) {
        return accel->pitch;
    } else {
        return NAN;
    }
}

float accel_get_roll(accel_t *accel) {
    if (accel->filter_stable) {
        return accel->roll;
    } else {
        return NAN;
    }
}

accel_gesture_display_orient_t accel_get_gest_display_orient(accel_t *accel) {
    if (accel->filter_stable) {
        return accel->gest_display_orient;
    } else {
        return AccelGestDisplayOrientUnknown;
    }
}

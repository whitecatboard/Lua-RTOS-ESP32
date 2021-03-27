#include "integral.h"

#include "simpson1_3.inc"

#include <math.h>
#include <stdlib.h>
#include <stdarg.h>

integral_t *integral_create(integral_type_t type, ...) {
    // Sanity checks
    if (type >= IntegralMax) {
        return NULL;
    }

    // Allocate space for integral
    integral_t *integral = calloc(1, sizeof(integral_t));
    if (integral == NULL) {
        return NULL;
    }

    va_list param;

    va_start(param, type);

    switch (type) {
        case IntegralSimpson1_3: {
            float h;

            h = (float)va_arg(param, double);

            integral_simpson1_3_create(integral, h);

            break;
        }

        default:
            break;
    }

    va_end(param);

    return integral;
}

void integral_reset(integral_t *integral) {
    if (integral == NULL) {
        return;
    }

    switch (integral->type) {
        case IntegralSimpson1_3: integral_simpson1_3_reset(integral); break;
        default:
            break;
    }
}

void integral_destroy(integral_t *integral) {
    if (integral == NULL) {
        return;
    }

    switch (integral->type) {
        case IntegralSimpson1_3: integral_simpson1_3_destroy(integral); break;
        default:
            break;
    }
}

float integral_value(integral_t *integral, float value) {
    float result = NAN;

    switch (integral->type) {
        case IntegralSimpson1_3: result = integral_simpson1_3_value(integral, value); break;
        default:
            break;
    }

    return result;
}

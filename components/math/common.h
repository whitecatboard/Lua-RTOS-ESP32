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
 * Lua RTOS, float math common functions
 *
 */

#ifndef MATH_MATH_H_
#define MATH_MATH_H_

#include <stdint.h>
#include <float.h>
#include <math.h>

/**
 * @brief Check if two given float numbers are near equals, taking into consideration
 *        the edge cases.
 *
 * @param a  First number to compare
 * @param b  Second number to compare
 * @param ep A pointer to an epsilon value. If it's NULL, FLT_EPSILON is taken.
 *
 * @return 1 if a ≈ b, 0 otherwise
 */
uint8_t flt_near_eq(float a, float b, float *ep);

/**
 * @brief Check if two given float numbers are greater or near equals, taking into
 *        consideration the edge cases.
 *
 * @param a First number to compare
 * @param b Second number to compare
 * @param ep A pointer to an epsilon value. If it's NULL, FLT_EPSILON is taken.
 *
 * @return 1 if a >≈ b, 0 otherwise
 */
uint8_t flt_greater_or_near_eq(float a, float b, float *ep);

/**
 * @brief Check if two given float numbers are less or near equals, taking into
 *        consideration the edge cases.
 *
 * @param a First number to compare
 * @param b Second number to compare
 * @param ep A pointer to an epsilon value. If it's NULL, FLT_EPSILON is taken.
 *
 * @return 1 if a >≈ b, 0 otherwise
 */
uint8_t flt_less_or_near_eq(float a, float b, float *ep);

#endif /* COMPONENTS_MATH_MATH_H_ */

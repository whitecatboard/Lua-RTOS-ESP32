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
 * Lua RTOS, On chip Hall effect sensor
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_HALL_INTERNAL

#define INTERNAL_HALL_SAMPLES 10

#include <drivers/sensor.h>
#include <drivers/adc.h>

// Sensor specification and registration
driver_error_t *internal_hall_setup(sensor_instance_t *unit);
driver_error_t *internal_hall_acquire(sensor_instance_t *unit, sensor_value_t *values);

static const sensor_t __attribute__((used,unused,section(".sensors"))) internal_hall_sensor = {
	.id = "INTERNAL_HALL",
	.interface = {
		{.type = INTERNAL_INTERFACE},
	},
	.data = {
		{.id = "raw", .type = SENSOR_DATA_INT},
	},
	.setup = internal_hall_setup,
	.acquire = internal_hall_acquire
};

/*
 * Operation functions
 */
driver_error_t *internal_hall_setup(sensor_instance_t *unit) {
	adc1_config_width(ADC_WIDTH_BIT_12);

	return NULL;
}

driver_error_t *internal_hall_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	int raw;
	float avg;
	int i;

	avg = 0;

	for(i=1; i <= INTERNAL_HALL_SAMPLES;i++) {
		raw = hall_sensor_read();

			avg = ((i - 1) * avg + raw) / i;
	}

	values->integerd.value = (int)avg;

	return NULL;
}
#endif
#endif

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
 * Lua RTOS, Push switch sensor
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR

#include <drivers/sensor.h>
#if CONFIG_LUA_RTOS_USE_SENSOR_3P_TOGGLE_SWITCH

driver_error_t *_3_pos_switch_setup(sensor_instance_t *unit);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) _3_pos_switch_sensor = {
    .id = "3P_TOGGLE_SWITCH",
    .interface = {
        {
            .type = GPIO_INTERFACE,

            .flags = SENSOR_FLAG_AUTO_ACQ | SENSOR_FLAG_ON_OFF | SENSOR_FLAG_ON_H(0) | SENSOR_FLAG_ON_L(1) |
                     SENSOR_FLAG_DEBOUNCING | SENSOR_FLAG_DEBOUNCING_THRESHOLD(10000)
        },
        {
            .type = GPIO_INTERFACE,

            .flags = SENSOR_FLAG_AUTO_ACQ | SENSOR_FLAG_ON_OFF | SENSOR_FLAG_ON_H(0) | SENSOR_FLAG_ON_L(2) |
                     SENSOR_FLAG_DEBOUNCING | SENSOR_FLAG_DEBOUNCING_THRESHOLD(10000)
        },
    },
    .interface_name = {"P1", "P2"},
    .data = {
        {.id = "pos", .type = SENSOR_DATA_INT},
    },
    .setup = _3_pos_switch_setup
};

driver_error_t *_3_pos_switch_setup(sensor_instance_t *unit) {
    // Get initial state
    unit->data[0].integerd.value = 0;

    uint8_t p1 = gpio_ll_pin_get(unit->setup[0].gpio.gpio);
    uint8_t p2 = gpio_ll_pin_get(unit->setup[1].gpio.gpio);

    if ((p1 == 1) && (p2 == 1)) {
        unit->data[0].integerd.value = 0;
    } else if ((p1 == 0) && (p2 == 1)) {
        unit->data[0].integerd.value = 1;
    } else if ((p1 == 1) && (p2 == 0)) {
        unit->data[0].integerd.value = 2;
    }

    unit->latch[0].value.integerd.value = unit->data[0].integerd.value;

    return NULL;
}

#endif
#endif

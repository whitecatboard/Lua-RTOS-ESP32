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
 * Lua RTOS, Relative rotary encoder sensor
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_HALL_FLOW_SENSOR

#define HALL_FLOW_TIMER_USEC 1000000

#include "esp_attr.h"

#include <string.h>

#include <sys/mutex.h>
#include <sys/time.h>

#include <drivers/sensor.h>

typedef struct {
    uint32_t count;
    uint32_t last;
} hall_flow_t;

driver_error_t *hall_flow_setup(sensor_instance_t *unit);
driver_error_t *hall_flow_unsetup(sensor_instance_t *unit);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) hall_flow_sensor = {
	.id = "HALL_FLOW",
	.interface = {
		{.type = GPIO_INTERFACE, .flags = (SENSOR_FLAG_CUSTOM_INTERFACE_INIT | SENSOR_FLAG_AUTO_ACQ)},
	},
	.data = {
        {.id = "q", .type = SENSOR_DATA_DOUBLE},
        {.id = "l", .type = SENSOR_DATA_DOUBLE},
        {.id = "f", .type = SENSOR_DATA_DOUBLE},
	},
    .properties = {
        {.id = "k", .type = SENSOR_DATA_DOUBLE},
    },
    .setup = hall_flow_setup,
	.unsetup = hall_flow_unsetup,
};

/*
 * Helper functions
 */

static void IRAM_ATTR flow_isr(void* arg) {
    uint32_t now = xthal_get_ccount();
    uint32_t cycles;

    // Get sensor instance and specific data
    sensor_instance_t *sensor = (sensor_instance_t *)arg;
    hall_flow_t *data = sensor->args;

    // Increment pulse count
    data->count++;

    // If first pulse, exit
    if (data->last == 0) {
        data->last = now;
        sensor->data[0].doubled.value = 0;
        return;
    }

    if (data->count == 20) {
        // Get CPU cycles since last interrupt
        if (now < data->last) {
            // Overflow
            cycles = (0xffffffff - data->last) + now;
        } else {
            cycles = now - data->last;
        }

        // Convert CPU cycles to microseconds
        double micros = ((double)(cycles)) / ((double)cpu_speed_hz() / (double)1000000);

        // Get frequency
        double freq = ((double)data->count * (double)1000000) / (double)micros;

        // Q
        sensor->data[0].doubled.value = freq / (double)sensor->properties[0].doubled.value;

        // L
        sensor->data[1].doubled.value += (sensor->data[0].doubled.value * micros) / (double)60000000;

        // Frequency
        sensor->data[2].doubled.value = freq;

        data->count = 0;
        data->last = now;
    }
}

/*
 * Operation functions
 */
driver_error_t *hall_flow_setup(sensor_instance_t *unit) {
    driver_error_t *error;
    hall_flow_t *sensor;

    unit->properties[0].doubled.value = 120;

    sensor = calloc(1, sizeof(hall_flow_t));
    if (!sensor) {
        return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    unit->args = sensor;

    if ((error = gpio_isr_attach(unit->setup[0].gpio.gpio, flow_isr, GPIO_INTR_POSEDGE, (void *)unit))) {
        return error;
    }

    return NULL;
}

driver_error_t *hall_flow_unsetup(sensor_instance_t *unit) {
	driver_error_t *error;

    if ((error = gpio_isr_detach(unit->setup[0].gpio.gpio))) {
        return error;
    }

	return NULL;
}
#endif
#endif

/*

 -- Attach to GPIO26
 s = sensor.attach("HALL_FLOW",26)

 -- Set K factor
 s:set("k", 120)

 -- Read current l/min
 s:read("q")

 -- Read total litters
 s:read("l")

 */

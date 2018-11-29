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
 * Lua RTOS, DHT22 sensor (temperature & humidity)
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_DHT22

#include "dhtxx.h"

#include <time.h>

#include <sys/driver.h>

#include <drivers/sensor.h>

driver_error_t *dht22_acquire(sensor_instance_t *unit, sensor_value_t *values);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) dht22_sensor = {
    .id = "DHT22",
    .interface = {
        {.type = GPIO_INTERFACE},
    },
    .data = {
        {.id = "temperature", .type = SENSOR_DATA_FLOAT},
        {.id = "humidity"   , .type = SENSOR_DATA_FLOAT},
    },
    .setup = dhtxx_setup,
    .acquire = dht22_acquire
};

driver_error_t *dht22_acquire(sensor_instance_t *unit, sensor_value_t *values) {
    uint8_t data[5]; // dht11 returns 5 bytes of data in each transfer

    driver_error_t *error = dhtxx_acquire(unit, 20, data);
    if (error != NULL) {
            return error;
    }

    values[0].floatd.value = (float)(((uint16_t)(data[2] & 0b01111111)) << 8 | (uint16_t)data[3]) / 10.0;

    // Apply temperature sing
    if (data[2] & 0b10000000) {
        values[0].floatd.value = -1 * values[0].floatd.value;
    }

    values[1].floatd.value = (float)(((uint16_t)data[0]) << 8 | (uint16_t)data[1]) / 10.0;

    // Next value can get in 2 seconds
    gettimeofday(&unit->next, NULL);
    unit->next.tv_sec += 2;

    return NULL;
}

#endif
#endif

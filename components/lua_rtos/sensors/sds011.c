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
 * Lua RTOS, SDS011 (Nova PM sensor)
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_SDS011

#include <drivers/sensor.h>
#include <drivers/uart.h>

driver_error_t *sds011_presetup(sensor_instance_t *unit);
driver_error_t *sds011_setup(sensor_instance_t *unit);

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) gps_sensor = {
	.id = "SDS011",
	.interface = {
		{.type = UART_INTERFACE, .flags = SENSOR_FLAG_AUTO_ACQ},
	},
	.data = {
		{.id = "PM2.5", .type = SENSOR_DATA_DOUBLE},
		{.id = "PM10" , .type = SENSOR_DATA_DOUBLE},
	},
	.presetup = sds011_presetup,
	.setup = sds011_setup,
	.acquire = NULL
};

static void sds011_task(void *args) {
	sensor_instance_t *unit = (sensor_instance_t *)args;
	uint8_t buff[10];
	uint8_t i, j, c, checksum;

	// SDS011 sends the information in a 10-byte packet:
	// AA CO XX XX XX XX XX XX XX AB
	i = 0;
	for(;;) {
		uart_read(unit->setup[0].uart.id, (char *)&c, portMAX_DELAY);
		if (i == 10) {
			for(j=1;j < 10;j++) {
				buff[j-1] = buff[j];
			}

			i = 9;
		}

		buff[i++] = c;

		if ((buff[0] == 0xaa) && (buff[1] == 0xc0) && (buff[9] == 0xab)) {
			// Checksum
			checksum = 0;
			for(j = 2;j <= 7;j++) {
				checksum = checksum + buff[j];
			}

			if (buff[8] == checksum) {
				sensor_lock(unit);
				unit->data[0].doubled.value = ((double)((buff[3] << 8) + buff[2]))/(double)10.0;
				unit->data[1].doubled.value = ((double)((buff[5] << 8) + buff[4]))/(double)10.0;
				sensor_unlock(unit);
			}
		}
	}
}

/*
 * Operation functions
 */
driver_error_t *sds011_presetup(sensor_instance_t *unit) {
	unit->setup[0].uart.speed = 9688;
	unit->setup[0].uart.data_bits = 8;
	unit->setup[0].uart.parity = 0;
	unit->setup[0].uart.stop_bits = 1;

	return NULL;
}

driver_error_t *sds011_setup(sensor_instance_t *unit) {
	xTaskCreatePinnedToCore(sds011_task, "sds011", configMINIMAL_STACK_SIZE, (void *)(unit), 21, NULL, 0);

	return NULL;
}

#endif
#endif


/*
  s = sensor.attach("SDS011", uart.UART1)

 */

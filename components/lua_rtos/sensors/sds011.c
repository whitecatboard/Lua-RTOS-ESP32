/*
 * Lua RTOS, SDS011 (Nova PM sensor)
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
 *
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
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
	.interface = UART_INTERFACE,
	.flags = SENSOR_FLAG_AUTO_ACQ,
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
		uart_read(unit->setup.uart.id, (char *)&c, portMAX_DELAY);
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
				unit->data[0].doubled.value = ((double)((buff[3] << 8) + buff[1]))/(double)10.0;
				unit->data[1].doubled.value = ((double)((buff[5] << 8) + buff[4]))/(double)10.0;
			}
		}
	}
}

/*
 * Operation functions
 */
driver_error_t *sds011_presetup(sensor_instance_t *unit) {
	unit->setup.uart.speed = 9688;
	unit->setup.uart.data_bits = 8;
	unit->setup.uart.parity = 0;
	unit->setup.uart.stop_bits = 1;

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

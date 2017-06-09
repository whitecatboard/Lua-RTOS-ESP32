/*
 * Lua RTOS, GPS sensor (geolocation)
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

#include "gps.h"
#include "nmea0183.h"

#include <math.h>

#include <sys/driver.h>

#include <drivers/sensor.h>
#include <drivers/uart.h>

// Sensor specification and registration
static const sensor_t __attribute__((used,unused,section(".sensors"))) gps_sensor = {
	.id = "GPS",
	.interface = UART_INTERFACE,
	.data = {
		{.id = "lon", .type = SENSOR_DATA_DOUBLE},
		{.id = "lat" , .type = SENSOR_DATA_DOUBLE},
		{.id = "sats", .type = SENSOR_DATA_INT},
	},
	.setup = gps_setup,
	.acquire = gps_acquire
};

static void gps(void *args) {
	int uart = (int)args;

	char sentence[MAX_NMA_SIZE];

	for(;;) {
		uart_reads(uart, sentence, 1, portMAX_DELAY);
		nmea_parse(sentence);
	}
}

/*
 * Operation functions
 */
driver_error_t *gps_setup(sensor_instance_t *unit) {
	xTaskCreatePinnedToCore(gps, "gps", configMINIMAL_STACK_SIZE, (void *)((int)unit->setup.uart.id), 21, NULL, 0);

	return NULL;
}

driver_error_t *gps_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	values[0].doubled.value  = nmea_lon();
	values[1].doubled.value  = nmea_lat();
	values[2].integerd.value = nmea_sats();

	return NULL;
}

#endif

/*

gps = sensor.attach("GPS", uart.UART1, 9600, 8, uart.PARNONE, uart.STOP1)

while true do
	lon, lat, sats = gps:read("all")

	print("lat: "..lat..", lon: "..lon..", sats: "..sats)
	tmr.delay(1)
end

*/

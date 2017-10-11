/*
 * Lua RTOS, cpu driver
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
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
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "rom/rtc.h"
#include <soc/dport_reg.h>
#include <soc/efuse_reg.h>
#include "soc/rtc.h"

#include <stdio.h>
#include <string.h>

#include <sys/syslog.h>
#include <sys/delay.h>
#include <sys/status.h>

#include <drivers/cpu.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <drivers/power_bus.h>
#include <drivers/wifi.h>

extern uint8_t flash_unique_id[8];

void _cpu_init() {
}

unsigned int cpu_port_number(unsigned int pin) {
	if ((pin <= 39) || (!EXTERNAL_GPIO)) {
		return 1;
	} else {
		return 2 + ((pin - EXTERNAL_GPIO_PINS) >> 3);
	}
}

uint8_t cpu_gpio_number(uint8_t pin) {
	if ((pin <= 39) || (!EXTERNAL_GPIO)) {
		return pin;
	} else {
		return ((pin - EXTERNAL_GPIO_PINS) % 7);
	}
}

unsigned int cpu_pin_number(unsigned int pin) {
	return pin;
}

gpio_pin_mask_t cpu_port_io_pin_mask(unsigned int port) {
	if ((port == 1) || (!EXTERNAL_GPIO)) {
		return GPIO_ALL;
	} else {
		return 0xff;
	}
}

unsigned int cpu_has_gpio(unsigned int port, unsigned int bit) {
	if (port == 1) {
		return (cpu_port_io_pin_mask(port) & (1 << bit));
	} else {
		if (bit < 8) {
			return 1;
		}

		return 0;
	}
}

unsigned int cpu_has_port(unsigned int port) {
	if (!EXTERNAL_GPIO) {
		return (port == 1);
	} else {
		return (port <= GPIO_PORTS);
	}
}

void cpu_model(char *buffer, int buflen) {
	snprintf(buffer, buflen, "ESP32 rev %d", cpu_revision());
}

uint32_t cpu_speed() {
	return rtc_clk_cpu_freq_value(rtc_clk_cpu_freq_get());
}

int cpu_revision() {
	return (REG_READ(EFUSE_BLK0_RDATA3_REG) >> EFUSE_RD_CHIP_VER_RESERVE_S) && EFUSE_RD_CHIP_VER_RESERVE_V;
}

void cpu_show_info() {
	char buffer[40];

	cpu_model(buffer, sizeof(buffer));
	if (!*buffer) {
		syslog(LOG_ERR, "cpu unknown CPU");
	} else {
		syslog(LOG_INFO, "cpu %s at %d Mhz", buffer, cpu_speed() / 1000000);
	}
}

void cpu_show_flash_info() {
	#if CONFIG_LUA_RTOS_READ_FLASH_UNIQUE_ID
	char buffer[17];

	snprintf(buffer, sizeof(buffer), 
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			flash_unique_id[0], flash_unique_id[1],
			flash_unique_id[2], flash_unique_id[3],
			flash_unique_id[4], flash_unique_id[5],
			flash_unique_id[6], flash_unique_id[7]
	);

	syslog(LOG_INFO, "flash EUI %s", buffer);
	#endif
}

void cpu_sleep(int seconds) {
	// Stop powerbus
	#if CONFIG_LUA_RTOS_USE_POWER_BUS
	pwbus_off();
	#endif

	// Stop all UART units. This is done for prevent strangers characters
	// on the console when CPU begins to enter in the sleep phase
	uart_stop(-1);

	// Put ESP32 in deep sleep mode
	if (status_get(STATUS_NEED_RTC_SLOW_MEM)) {
		esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
	}

	esp_deep_sleep(seconds * 1000000LL);
}

void cpu_deepsleep() {
	wifi_stop();

	// Stop powerbus
	#if CONFIG_LUA_RTOS_USE_POWER_BUS
	pwbus_off();
	#endif

	// Stop all UART units. This is done for prevent strangers characters
	// on the console when CPU begins to enter in the sleep phase
	uart_stop(-1);

	// Put ESP32 in deep sleep mode
	if (status_get(STATUS_NEED_RTC_SLOW_MEM)) {
		esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
	}

/*
 * esp_deep_sleep does NOT shut down WiFi, BT, and higher level protocol
 * connections gracefully.
 * Make sure relevant WiFi and BT stack functions are called to close any
 * connections and deinitialize the peripherals. These include:
 *     - esp_bluedroid_disable
 *     - esp_bt_controller_disable
 *     - esp_wifi_stop
 *
 * This function does not return.
 */

	esp_deep_sleep_start();
}

void cpu_reset() {
	esp_restart();
}

int cpu_reset_reason() {
	return rtc_get_reset_reason(0);
}

int cpu_wakeup_reason() {
	return esp_sleep_get_wakeup_cause();
}


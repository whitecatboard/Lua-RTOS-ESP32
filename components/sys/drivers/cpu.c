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
 * Lua RTOS, CPU driver
 *
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

uint32_t cpu_speed_mhz() {
  rtc_cpu_freq_config_t config;
  rtc_clk_cpu_freq_get_config(&config);

  return config.freq_mhz;
}

uint32_t cpu_speed_hz() {
  return cpu_speed_mhz() * 1000000;
}

uint32_t cpu_speed() {
  return cpu_speed_hz();
}

int cpu_revision() {
    esp_chip_info_t info;
    esp_chip_info(&info);
    return info.revision;
}

void cpu_show_info() {
	char buffer[40];

	cpu_model(buffer, sizeof(buffer));
	if (!*buffer) {
		syslog(LOG_INFO, "cpu unknown CPU");
	} else {
		esp_chip_info_t info;
		esp_chip_info(&info);

		syslog(LOG_INFO, "cpu %s at %d Mhz, %i %s%s%s%s%s", buffer, cpu_speed_mhz(), info.cores, info.cores>1 ? "Cores":"Core",
					info.features & CHIP_FEATURE_EMB_FLASH ? ", flash memory":"",
					info.features & CHIP_FEATURE_WIFI_BGN ? ", 2.4GHz WiFi":"",
					info.features & CHIP_FEATURE_BLE ? ", Bluetooth LE":"",
					info.features & CHIP_FEATURE_BT ? ", Bluetooth Classic":"");
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
	#if (CONFIG_LUA_RTOS_POWER_BUS_PIN >= 0)
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
	#if (CONFIG_LUA_RTOS_POWER_BUS_PIN >= 0)
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


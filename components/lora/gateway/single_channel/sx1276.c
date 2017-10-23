/*
 * Lua RTOS, simple channel LoRa WAN gateway
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
#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LORA_DEVICE_TYPE_SINGLE_CHAN_GATEWAY

#include "sx1276.h"

#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <drivers/power_bus.h>

void sx1276_reset(uint8_t val) {
	#if CONFIG_LUA_RTOS_USE_POWER_BUS
		if (val == 1) {
			pwbus_off();
			delay(1);
			pwbus_on();
		} else if (val == 0) {
			pwbus_off();
			delay(1);
			pwbus_on();
		} else {
			delay(5);
		}
	#else
		if (val == 1) {
			gpio_pin_output(CONFIG_LUA_RTOS_LORA_NODE_RST);
			gpio_pin_set(CONFIG_LUA_RTOS_LORA_NODE_RST);
		} else if (val == 0) {
			gpio_pin_output(CONFIG_LUA_RTOS_LORA_NODE_RST);
			gpio_pin_clr(CONFIG_LUA_RTOS_LORA_NODE_RST);
		} else {
			gpio_pin_input(CONFIG_LUA_RTOS_LORA_NODE_RST);
		}
	#endif
}

void stx1276_read_reg(int spi_device, uint8_t addr, uint8_t *data) {
	spi_ll_select(spi_device);
	spi_ll_transfer(spi_device, addr & 0x7f, NULL);
	spi_ll_transfer(spi_device, 0xff, data);
	spi_ll_deselect(spi_device);
}

void stx1276_write_reg(int spi_device, uint8_t addr, uint8_t data) {
	spi_ll_select(spi_device);
	spi_ll_transfer(spi_device, addr | 0x80, NULL);
	spi_ll_transfer(spi_device, data, NULL);
	spi_ll_deselect(spi_device);
}

void stx1276_read_buff(int spi_device, uint8_t addr, uint8_t *data, uint8_t len) {
	spi_ll_select(spi_device);
	spi_ll_transfer(spi_device, addr & 0x7f, NULL);
	spi_ll_bulk_read(spi_device, len, data);
	spi_ll_deselect(spi_device);
}

#endif

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
 * Lua RTOS, simple channel LoRa WAN gateway
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276 || CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272

#include "sx1276.h"

#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <drivers/power_bus.h>

void sx1276_reset(uint8_t val) {
	#if (CONFIG_LUA_RTOS_POWER_BUS_PIN >= 0)
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
		#if CONFIG_LUA_RTOS_LORA_RST >= 0
			if (val == 1) {
				gpio_pin_output(CONFIG_LUA_RTOS_LORA_RST);
				gpio_pin_set(CONFIG_LUA_RTOS_LORA_RST);
			} else if (val == 0) {
				gpio_pin_output(CONFIG_LUA_RTOS_LORA_RST);
				gpio_pin_clr(CONFIG_LUA_RTOS_LORA_RST);
			} else {
				gpio_pin_input(CONFIG_LUA_RTOS_LORA_RST);
			}
		#endif
	#endif
}

void IRAM_ATTR stx1276_read_reg(int spi_device, uint8_t addr, uint8_t *data) {
	spi_ll_select(spi_device);
	spi_ll_transfer(spi_device, addr & 0x7f, NULL);
	spi_ll_transfer(spi_device, 0xff, data);
	spi_ll_deselect(spi_device);
}

void IRAM_ATTR stx1276_write_reg(int spi_device, uint8_t addr, uint8_t data) {
	spi_ll_select(spi_device);
	spi_ll_transfer(spi_device, addr | 0x80, NULL);
	spi_ll_transfer(spi_device, data, NULL);
	spi_ll_deselect(spi_device);
}

void IRAM_ATTR stx1276_read_buff(int spi_device, uint8_t addr, uint8_t *data, uint8_t len) {
	spi_ll_select(spi_device);
	spi_ll_transfer(spi_device, addr & 0x7f, NULL);
	spi_ll_bulk_read(spi_device, len, data);
	spi_ll_deselect(spi_device);
}

#endif

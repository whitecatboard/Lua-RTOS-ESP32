/*
 * Lua RTOS, PCF8544 driver
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

#if CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

#include "freertos/FreeRTOS.h"

#include <string.h>

#include <sys/delay.h>
#include <sys/driver.h>
#include <sys/syslog.h>

#include <gdisplay/gdisplay.h>

#include <drivers/pcd8544.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>

#include <drivers/gdisplay.h>

/*
 * Operation functions
 */
void pcd8544_ll_clear() {
	uint8_t *buff = (uint8_t *)gdisplay_ll_get_buffer();
	uint32_t buff_size = gdisplay_ll_get_buffer_size();

	memset(buff,0x00,sizeof(uint8_t) * buff_size);
	pcd8544_update(0,0,LCDWIDTH-1,LCDHEIGHT-1, buff);
}

driver_error_t *pcd8544_init(uint8_t chipset, uint8_t orient, uint8_t address) {
	driver_error_t *error;
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	caps->addr_window = pcd8544_addr_window;
	caps->on = pcd8544_on;
	caps->off = pcd8544_off;
	caps->invert = pcd8544_invert;
	caps->orientation = pcd8544_set_orientation;
	caps->touch_get = NULL;
	caps->touch_cal = NULL;
	caps->bytes_per_pixel = 0;
	caps->rdepth = 0;
	caps->gdepth = 0;
	caps->bdepth = 0;
	caps->phys_width = LCDWIDTH;
	caps->phys_height = LCDHEIGHT;
	caps->interface = GDisplaySPIInterface;
	caps->monochrome_white = 0;

    // Init SPI bus
	if (caps->device == -1) {
		if ((error = spi_setup(CONFIG_LUA_RTOS_GDISPLAY_TP_SPI, 1, CONFIG_LUA_RTOS_GDISPLAY_CS, 0, 4000000, SPI_FLAG_WRITE | SPI_FLAG_READ, &caps->device))) {
			return error;
		}
	}

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unit_lock_error_t *lock_error = NULL;
	if ((error = spi_lock_bus_resources(CONFIG_LUA_RTOS_GDISPLAY_SPI, DRIVER_ALL_FLAGS))) {
		return error;
	}

	if ((lock_error = driver_lock(GDISPLAY_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_GDISPLAY_CMD, DRIVER_ALL_FLAGS, "gdisplay - PCD8544"))) {
		return driver_lock_error(GDISPLAY_DRIVER, lock_error);
	}

	if ((lock_error = driver_lock(GDISPLAY_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_GDISPLAY_RESET, DRIVER_ALL_FLAGS, "gdisplay - PCD8544"))) {
		return driver_lock_error(GDISPLAY_DRIVER, lock_error);
	}
#endif

	// setup command pin
	gpio_pin_output(CONFIG_LUA_RTOS_GDISPLAY_CMD);

	// setup reset pin
	gpio_pin_output(CONFIG_LUA_RTOS_GDISPLAY_RESET);
	gpio_ll_pin_set(CONFIG_LUA_RTOS_GDISPLAY_RESET);

	// Reset
	gpio_ll_pin_clr(CONFIG_LUA_RTOS_GDISPLAY_RESET);
	delay(1);
	gpio_ll_pin_set(CONFIG_LUA_RTOS_GDISPLAY_RESET);

	// set extended
	gdisplay_ll_command(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION);  // extended instruction set control (H=1)

    // set bias system (1:48)
	gdisplay_ll_command(PCD8544_SETBIAS | 0x03);

    // default Vop (3.06 + 66 * 0.06 = 7V)
	gdisplay_ll_command(PCD8544_SETVOP | 0x42);

	// set normal
	gdisplay_ll_command(PCD8544_FUNCTIONSET);

    // all display segments on
	gdisplay_ll_command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYALLON);

	// Allocate buffer
	if (!gdisplay_ll_allocate_buffer((LCDWIDTH * LCDHEIGHT) / 8)) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	pcd8544_ll_clear();

    // enable lcd
    gdisplay_ll_command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYBLANK);
    gdisplay_ll_command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);

	syslog(LOG_INFO, "LCD PCD8544 at spi%d, cs=%s%d", CONFIG_LUA_RTOS_GDISPLAY_TP_SPI,
		gpio_portname(CONFIG_LUA_RTOS_GDISPLAY_CS), gpio_name(CONFIG_LUA_RTOS_GDISPLAY_CS)
	);

	pcd8544_set_orientation(orient);

	return NULL;
}


void pcd8544_addr_window(uint8_t write, int x0, int y0, int x1, int y1) {
	gdisplay_ll_command(PCD8544_SETYADDR);
	gdisplay_ll_command(PCD8544_SETXADDR);
}

void pcd8544_update(int x0, int y0, int x1, int y1, uint8_t *buffer) {
	uint8_t *dst = (buffer?buffer:gdisplay_ll_get_buffer());
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	pcd8544_addr_window(1, x0, y0, x1, y1);

	gpio_ll_pin_set(CONFIG_LUA_RTOS_GDISPLAY_CMD);
	spi_ll_select(caps->device);
	spi_ll_bulk_write(caps->device, sizeof(uint8_t) * ((LCDWIDTH * LCDHEIGHT) / 8), dst);
	spi_ll_deselect(caps->device);
}

void pcd8544_set_orientation(uint8_t orientation) {
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	caps->orient = orientation;

	if ((caps->orient == LANDSCAPE) || (caps->orient == LANDSCAPE_FLIP)) {
		caps->width = LCDWIDTH;
		caps->height = LCDHEIGHT;
	} else {
		caps->width = LCDHEIGHT;
		caps->height = LCDWIDTH;
	}
}

void pcd8544_on() {
	gdisplay_ll_command(PCD8544_FUNCTIONSET);
    gdisplay_ll_command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);
}

void pcd8544_off() {
	gdisplay_ll_command(PCD8544_FUNCTIONSET | PCD8544_POWERDOWN);
	gdisplay_ll_command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYBLANK);
}

void pcd8544_invert(uint8_t on) {
	if (on) {
		gdisplay_ll_command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYINVERTED);
	} else {
		gdisplay_ll_command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);
	}
}

#endif

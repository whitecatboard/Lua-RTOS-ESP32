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
 * Lua RTOS, ST7735 driver
 *
 */

/*
 * This driver is inspired and takes code from the following projects:
 *
 * Boris Lovošević, tft driver for Lua RTOS:
 *
 * https://github.com/loboris/Lua-RTOS-ESP32-lobo/tree/master/components/lua_rtos/Lua/modules/screen
 *
 * Adafruit ST7735 library:
 *
 * https://github.com/adafruit/Adafruit-ST7735-Library
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

#include <sys/driver.h>
#include <sys/syslog.h>

#include <gdisplay/gdisplay.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <drivers/gdisplay.h>

#include <drivers/st7735.h>

// Current chipset
static uint8_t chipset;

// ST7735 variants
typedef struct {
	uint8_t width;
	uint8_t height;
	uint8_t colstart;
	uint8_t rowstart;
	uint8_t order;
} st7735_variant_t;

static const st7735_variant_t variant[] = {
	{128, 160, 0 ,  0, ST7735_MADCTL_RGB}, // 1.8" BLACK
	{128, 160, 0 ,  0, ST7735_MADCTL_RGB}, // 1.8" BLUE
	{128, 160, 0 ,  0, ST7735_MADCTL_RGB}, // 1.8" GREEN
	{128, 128, 2 ,  3, ST7735_MADCTL_RGB}, // 1.44" GREEN
	{160, 80 , 24,  0, ST7735_MADCTL_RGB}  // 0.96" BLACK
};


// Initialization commands for 7735B screens
// -----------------------------------------
static const uint8_t Bcmd[] = {
  18,						// 18 commands in list:
  ST7735_SWRESET,   DELAY,	//  1: Software reset, no args, w/delay
  50,						//     50 ms delay
  ST7735_SLPOUT ,   DELAY,	//  2: Out of sleep mode, no args, w/delay
  255,						//     255 = 500 ms delay
  ST7735_COLMOD , 1+DELAY,	//  3: Set color mode, 1 arg + delay:
  0x05,						//     16-bit color 5-6-5 color format
  10,						//     10 ms delay
  ST7735_FRMCTR1, 3+DELAY,	//  4: Frame rate control, 3 args + delay:
  0x00,						//     fastest refresh
  0x06,						//     6 lines front porch
  0x03,						//     3 lines back porch
  10,						//     10 ms delay
  ST7735_MADCTL , 1      ,		//  5: Memory access ctrl (directions), 1 arg:
  0x08,						//     Row addr/col addr, bottom to top refresh
  ST7735_DISSET5, 2      ,	//  6: Display settings #5, 2 args, no delay:
  0x15,						//     1 clk cycle nonoverlap, 2 cycle gate
  // rise, 3 cycle osc equalize
  0x02,						//     Fix on VTL
  ST7735_INVCTR , 1      ,	//  7: Display inversion control, 1 arg:
  0x0,						//     Line inversion
  ST7735_PWCTR1 , 2+DELAY,	//  8: Power control, 2 args + delay:
  0x02,						//     GVDD = 4.7V
  0x70,						//     1.0uA
  10,						//     10 ms delay
  ST7735_PWCTR2 , 1      ,	//  9: Power control, 1 arg, no delay:
  0x05,						//     VGH = 14.7V, VGL = -7.35V
  ST7735_PWCTR3 , 2      ,	// 10: Power control, 2 args, no delay:
  0x01,						//     Opamp current small
  0x02,						//     Boost frequency
  ST7735_VMCTR1 , 2+DELAY,	// 11: Power control, 2 args + delay:
  0x3C,						//     VCOMH = 4V
  0x38,						//     VCOML = -1.1V
  10,						//     10 ms delay
  ST7735_PWCTR6 , 2      ,	// 12: Power control, 2 args, no delay:
  0x11, 0x15,
  ST7735_GMCTRP1,16      ,	// 13: Magical unicorn dust, 16 args, no delay:
  0x09, 0x16, 0x09, 0x20,	//     (seriously though, not sure what
  0x21, 0x1B, 0x13, 0x19,	//      these config values represent)
  0x17, 0x15, 0x1E, 0x2B,
  0x04, 0x05, 0x02, 0x0E,
  ST7735_GMCTRN1,16+DELAY,	// 14: Sparkles and rainbows, 16 args + delay:
  0x0B, 0x14, 0x08, 0x1E,	//     (ditto)
  0x22, 0x1D, 0x18, 0x1E,
  0x1B, 0x1A, 0x24, 0x2B,
  0x06, 0x06, 0x02, 0x0F,
  10,						//     10 ms delay
  ST7735_CASET  , 4      , 	// 15: Column addr set, 4 args, no delay:
  0x00, 0x02,				//     XSTART = 2
  0x00, 0x81,				//     XEND = 129
  ST7735_PASET  , 4      , 	// 16: Row addr set, 4 args, no delay:
  0x00, 0x02,				//     XSTART = 1
  0x00, 0x81,				//     XEND = 160
  ST7735_NORON  ,   DELAY,	// 17: Normal display on, no args, w/delay
  10,						//     10 ms delay
  ST7735_DISPON ,   DELAY,  	// 18: Main screen turn on, no args, w/delay
  255						//     255 = 500 ms delay
};

// Init for 7735R, part 1 (red or green tab)
// -----------------------------------------
static const uint8_t  Rcmd1[] = {
  15,						// 15 commands in list:
  ST7735_SWRESET,   DELAY,	//  1: Software reset, 0 args, w/delay
  150,						//     150 ms delay
  ST7735_SLPOUT ,   DELAY,	//  2: Out of sleep mode, 0 args, w/delay
  255,						//     500 ms delay
  ST7735_FRMCTR1, 3      ,	//  3: Frame rate ctrl - normal mode, 3 args:
  0x00, 0x06, 0x03,			//     Rate = fosc/(1x2+40) * (LINE+2C+2D)
  ST7735_FRMCTR2, 3      ,	//  4: Frame rate control - idle mode, 3 args:
  0x00, 0x06, 0x03,			//     Rate = fosc/(1x2+40) * (LINE+2C+2D)
  ST7735_FRMCTR3, 6      ,	//  5: Frame rate ctrl - partial mode, 6 args:
  0x00, 0x06, 0x03,			//     Dot inversion mode
  0x00, 0x06, 0x03,			//     Line inversion mode
  ST7735_INVCTR , 1      ,	//  6: Display inversion ctrl, 1 arg, no delay:
  0x07,						//     No inversion
  ST7735_PWCTR1 , 3      ,	//  7: Power control, 3 args, no delay:
  0xA2,
  0x02,						//     -4.6V
  0x84,						//     AUTO mode
  ST7735_PWCTR2 , 1      ,	//  8: Power control, 1 arg, no delay:
  0xC5,						//     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
  ST7735_PWCTR3 , 2      ,	//  9: Power control, 2 args, no delay:
  0x0A,						//     Opamp current small
  0x00,						//     Boost frequency
  ST7735_PWCTR4 , 2      ,	// 10: Power control, 2 args, no delay:
  0x8A,						//     BCLK/2, Opamp current small & Medium low
  0x2A,
  ST7735_PWCTR5 , 2      ,	// 11: Power control, 2 args, no delay:
  0x8A, 0xEE,
  ST7735_VMCTR1 , 1      ,	// 12: Power control, 1 arg, no delay:
  0x0E,
  ST7735_INVOFF , 0      ,		// 13: Don't invert display, no args, no delay
  ST7735_MADCTL , 1      ,		// 14: Memory access control (directions), 1 arg:
  0xC0,						//     row addr/col addr, bottom to top refresh, RGB order
  ST7735_COLMOD , 1+DELAY,	//  15: Set color mode, 1 arg + delay:
  0x05,						//     16-bit color 5-6-5 color format
  10						//     10 ms delay
};

// Init for 7735R, part 2 (green tab only)
// ---------------------------------------
static const uint8_t Rcmd2green[] = {
  2,						//  2 commands in list:
  ST7735_CASET  , 4      ,		//  1: Column addr set, 4 args, no delay:
  0x00, 0x02,				//     XSTART = 0
  0x00, 0x7F+0x02,			//     XEND = 129
  ST7735_PASET  , 4      ,	    //  2: Row addr set, 4 args, no delay:
  0x00, 0x01,				//     XSTART = 0
  0x00, 0x9F+0x01			//     XEND = 160
};

// Init for 7735R, part 2 (red tab only)
// -------------------------------------
static const uint8_t Rcmd2red[] = {
  2,						//  2 commands in list:
  ST7735_CASET  , 4      ,	    //  1: Column addr set, 4 args, no delay:
  0x00, 0x00,				//     XSTART = 0
  0x00, 0x7F,				//     XEND = 127
  ST7735_PASET  , 4      ,	    //  2: Row addr set, 4 args, no delay:
  0x00, 0x00,				//     XSTART = 0
  0x00, 0x9F				//     XEND = 159
};

// Init for 7735R, part 3 (red or green tab)
// -----------------------------------------
static const uint8_t Rcmd3[] = {
  4,						//  4 commands in list:
  ST7735_GMCTRP1, 16      ,	//  1: Magical unicorn dust, 16 args, no delay:
  0x02, 0x1c, 0x07, 0x12,
  0x37, 0x32, 0x29, 0x2d,
  0x29, 0x25, 0x2B, 0x39,
  0x00, 0x01, 0x03, 0x10,
  ST7735_GMCTRN1, 16      ,	//  2: Sparkles and rainbows, 16 args, no delay:
  0x03, 0x1d, 0x07, 0x06,
  0x2E, 0x2C, 0x29, 0x2D,
  0x2E, 0x2E, 0x37, 0x3F,
  0x00, 0x00, 0x02, 0x10,
  ST7735_NORON  ,    DELAY,	//  3: Normal display on, no args, w/delay
  10,						//     10 ms delay
  ST7735_DISPON ,    DELAY,	//  4: Main screen turn on, no args w/delay
  100						//     100 ms delay
};

static const uint8_t Rcmd2green144[] = {              // Init for 7735R, part 2 (green 1.44 tab)
  2,                        //  2 commands in list:
  ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
    0x00, 0x00,             //     XSTART = 0
    0x00, 0x7F,             //     XEND = 127
	ST7735_PASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
    0x00, 0x00,             //     XSTART = 0
    0x00, 0x7F };           //     XEND = 127

static const uint8_t Rcmd2green160x80[] = {              // Init for 7735R, part 2 (mini 160x80)
   2,                        //  2 commands in list:
   ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
    0x00, 0x00,             //     XSTART = 0
    0x00, 0x7F,             //     XEND = 79
	ST7735_PASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
    0x00, 0x00,             //     XSTART = 0
    0x00, 0x9F };           //     XEND = 159

/*
 * Helper functions
 */

// Initialization code common to both 'B' and 'R' type displays
//-----------------------------------------------------
static void ST7735_commonInit(const uint8_t *cmdList) {
	// toggle RST low to reset; CS low so it'll listen to us
#if CONFIG_LUA_RTOS_GDISPLAY_RESET == -1
  gdisplay_ll_command(ST7735_SWRESET);
  vTaskDelay(130 / portTICK_RATE_MS);
#else
  gpio_pin_set(CONFIG_LUA_RTOS_GDISPLAY_RESET);
  vTaskDelay(10 / portTICK_RATE_MS);
  gpio_pin_clr(CONFIG_LUA_RTOS_GDISPLAY_RESET);
  vTaskDelay(50 / portTICK_RATE_MS);
  gpio_pin_set(CONFIG_LUA_RTOS_GDISPLAY_RESET);
  vTaskDelay(130 / portTICK_RATE_MS);
#endif
  if(cmdList) gdisplay_ll_command_list((uint8_t *)cmdList);
}

// Initialization for ST7735B screens
//------------------------------
static void ST7735_initB(void) {
  ST7735_commonInit(Bcmd);
}

/*
 * Operation functions
 */

driver_error_t *st7735_init(uint8_t chip, uint8_t orientation, uint8_t address) {
	driver_error_t *error;
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	caps->addr_window = st7735_addr_window;
	caps->on = st7735_on;
	caps->off = st7735_off;
	caps->invert = st7735_invert;
	caps->orientation = st7735_set_orientation;
	caps->touch_get = NULL;
	caps->touch_cal = NULL;
	caps->bytes_per_pixel = 2;
	caps->rdepth = 5;
	caps->gdepth = 6;
	caps->bdepth = 5;
	caps->phys_width  = variant[chipset - CHIPSET_ST7735_VARIANT_OFFSET].height;
	caps->phys_height = variant[chipset - CHIPSET_ST7735_VARIANT_OFFSET].width;
	caps->interface = GDisplaySPIInterface;

	// Store chipset
	chipset = chip;

    // Init SPI bus
	if (caps->device == -1) {
		if ((error = spi_setup(CONFIG_LUA_RTOS_GDISPLAY_SPI, 1, CONFIG_LUA_RTOS_GDISPLAY_CS, 0, 30000000, SPI_FLAG_WRITE | SPI_FLAG_NO_DMA, &caps->device))) {
			return error;
		}
	}

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unit_lock_error_t *lock_error = NULL;
	if ((error = spi_lock_bus_resources(CONFIG_LUA_RTOS_GDISPLAY_SPI, DRIVER_ALL_FLAGS))) {
		return error;
	}

	if ((lock_error = driver_lock(GDISPLAY_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_GDISPLAY_CMD, DRIVER_ALL_FLAGS, "gdisplay - ST7735"))) {
		return driver_lock_error(GDISPLAY_DRIVER, lock_error);
	}
#endif

	// setup command pin
	gpio_pin_output(CONFIG_LUA_RTOS_GDISPLAY_CMD);

#if CONFIG_LUA_RTOS_GDISPLAY_RESET != -1
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	if ((lock_error = driver_lock(GDISPLAY_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_GDISPLAY_RESET, DRIVER_ALL_FLAGS, "gdisplay - ST7735"))) {
		return driver_lock_error(GDISPLAY_DRIVER, lock_error);
	}
#endif

	// setup reset pin
	gpio_pin_output(CONFIG_LUA_RTOS_GDISPLAY_RESET);
	gpio_ll_pin_clr(CONFIG_LUA_RTOS_GDISPLAY_RESET);
#endif

    // Init display
    uint8_t dt;
    switch (chipset) {
		case CHIPSET_ST7735:
			ST7735_commonInit(Rcmd1);
			gdisplay_ll_command_list((uint8_t *)Rcmd2red);
			gdisplay_ll_command_list((uint8_t *)Rcmd3);
			gdisplay_ll_command(ST7735_MADCTL);
			dt = 0xC0;
			gdisplay_ll_data(&dt, 1);
			break;

		case CHIPSET_ST7735B:
			ST7735_initB();
			break;

		case CHIPSET_ST7735G:
			ST7735_commonInit(Rcmd1);
			gdisplay_ll_command_list((uint8_t *)Rcmd2green);
			gdisplay_ll_command_list((uint8_t *)Rcmd3);
			break;

		case CHIPSET_ST7735G_144:
			ST7735_commonInit(Rcmd1);
			gdisplay_ll_command_list(Rcmd2green144);
			break;

		case CHIPSET_ST7735_096:
			ST7735_commonInit(Rcmd1);
			gdisplay_ll_command_list(Rcmd2green160x80);
			break;
    }

    st7735_set_orientation(orientation);

	// Allocate buffer
	if (!gdisplay_ll_allocate_buffer(ST7735_BUFFER)) {
		return driver_error(GDISPLAY_DRIVER, GDISPLAY_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Clear screen (black)
    st7735_clear(GDISPLAY_BLACK);

    gdisplay_ll_command(ST7735_DISPON); // Display On

    return NULL;
}

void st7735_addr_window(uint8_t write, int x0, int y0, int x1, int y1) {
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();
	uint32_t wd;

    if ((x0 > caps->width) || (y0 > caps->height) || (x1 > caps->width) || (y1 > caps->height)) {
        return;
    }

    x0 += caps->xstart;
    x1 += caps->xstart;

    y0 += caps->ystart;
    y1 += caps->ystart;

    wd  = (uint32_t)(x0 >> 8);
	wd |= (uint32_t)(x0 & 0xff) << 8;
	wd |= (uint32_t)(x1 >> 8) << 16;
	wd |= (uint32_t)(x1 & 0xff) << 24;

    gdisplay_ll_command(ST7735_CASET); // Column Address Set
    gdisplay_ll_data32(wd);

	wd  = (uint32_t)(y0 >> 8);
	wd |= (uint32_t)(y0 & 0xff) << 8;
	wd |= (uint32_t)(y1 >> 8) << 16;
	wd |= (uint32_t)(y1 & 0xff) << 24;

    gdisplay_ll_command(ST7735_PASET); // Row Address Set
    gdisplay_ll_data32(wd);

    gdisplay_ll_command((write?ST7735_RAMWR:ST7735_RAMRD));
}

void st7735_set_orientation(uint8_t m) {
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();
	uint8_t orientation = m & 3; // can't be higher than 3
	uint8_t madctl = 0;

	if ((orientation & 1)) {
		caps->width  = variant[chipset - CHIPSET_ST7735_VARIANT_OFFSET].height;
		caps->height = variant[chipset - CHIPSET_ST7735_VARIANT_OFFSET].width;
		caps->ystart = variant[chipset - CHIPSET_ST7735_VARIANT_OFFSET].colstart;
		caps->xstart = variant[chipset - CHIPSET_ST7735_VARIANT_OFFSET].rowstart;
	}
	else {
		caps->width  = variant[chipset - CHIPSET_ST7735_VARIANT_OFFSET].width;
		caps->height = variant[chipset - CHIPSET_ST7735_VARIANT_OFFSET].height;
		caps->xstart = variant[chipset - CHIPSET_ST7735_VARIANT_OFFSET].colstart;
		caps->ystart = variant[chipset - CHIPSET_ST7735_VARIANT_OFFSET].rowstart;
	}

	switch (orientation) {
	  case PORTRAIT:
		madctl = (ST7735_MADCTL_MX | ST7735_MADCTL_MY | variant[chipset -CHIPSET_ST7735_VARIANT_OFFSET].order);
		break;
	  case LANDSCAPE:
		madctl = (ST7735_MADCTL_MY | ST7735_MADCTL_MV | variant[chipset -CHIPSET_ST7735_VARIANT_OFFSET].order);
		break;
	  case PORTRAIT_FLIP:
		madctl = (variant[chipset -CHIPSET_ST7735_VARIANT_OFFSET].order);
		break;
	  case LANDSCAPE_FLIP:
		madctl = (ST7735_MADCTL_MX | ST7735_MADCTL_MV | variant[chipset -CHIPSET_ST7735_VARIANT_OFFSET].order);
		break;
	}

	gdisplay_ll_command(ST7735_MADCTL);
	gdisplay_ll_data(&madctl, 1);
}

void st7735_on() {
	gdisplay_ll_command(ST7735_DISPON);
}

void st7735_off() {
	gdisplay_ll_command(ST7735_DISPOFF);
}

void st7735_invert(uint8_t on) {
	if (on) {
		gdisplay_ll_command(ST7735_INVONN);
	} else {
		gdisplay_ll_command(ST7735_INVOFF);
	}
}

void st7735_color(uint16_t *color, uint32_t len) {
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();
	uint16_t *buff = (uint16_t *)gdisplay_ll_get_buffer();
	uint32_t buff_size = gdisplay_ll_get_buffer_size();
	uint16_t *buffer;

	int i;

	if (len > 0) {
		buffer = buff;
		for(i=0;i < buff_size;i++) {
			buffer[i] = *color;
		}
	} else {
		buffer = color;
	}

    // Set DC to 1 (data mode);
	gpio_ll_pin_set(CONFIG_LUA_RTOS_GDISPLAY_CMD);
	spi_ll_select(caps->device);
	if (len > 0) {
		int clen;
		while (len) {
			clen = (len > buff_size?buff_size:len);
			spi_ll_bulk_write16(caps->device, clen, buffer);
			len = len - clen;
		}
	} else {
		spi_ll_bulk_write16(caps->device, 1, buffer);
	}
	spi_ll_deselect(caps->device);

	gdisplay_ll_invalidate_buffer();
}

void st7735_clear(uint32_t color) {
	gdisplay_caps_t *caps = gdisplay_ll_get_caps();

	st7735_addr_window(1, 0, 0, caps->width - 1, caps->height - 1);
	st7735_color((uint16_t *)&color, caps->width * caps->height);
}

#endif

/*
 * Whitecat, st7735 controller
 *
 * Copyright (C) 2015 - 2016
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

#include "whitecat.h"
#include "ST7735S.h"

#include <sys/syslog.h>
#include <sys/drivers/spi.h>
#include <sys/drivers/gpio.h>
#include <sys/delay.h>

#define DISPLAY_SPEED 40000

int tftwidth  = 128;
int tftheight = 160;

int st7735_width() {
    return tftwidth;
}

int st7735_height() {
    return tftheight;
}

void st7735_begindata() {
    gpio_pin_set(DISPLAY_RS);
    spi_select(DISPLAY_SPI);        
}

void st7735_enddata() {
    spi_deselect(DISPLAY_SPI);    
}

void st7735_sendcolor(int color) {
    spi_transfer(DISPLAY_SPI, color >> 8);    
    spi_transfer(DISPLAY_SPI, color);    
}

void st7735_command(int cmd) {
    gpio_pin_clr(DISPLAY_RS);
    spi_select(DISPLAY_SPI);    
    spi_transfer(DISPLAY_SPI, cmd);
    spi_deselect(DISPLAY_SPI);    
}

void st7735_data(int data) {
    gpio_pin_set(DISPLAY_RS);
    spi_select(DISPLAY_SPI);    
    spi_transfer(DISPLAY_SPI, data);
    spi_deselect(DISPLAY_SPI);    
}

void st7735_reset() {
    spi_select(DISPLAY_SPI);    
    gpio_pin_set(DISPLAY_RE);
    delay(500);
    gpio_pin_clr(DISPLAY_RE);
    delay(500);
    gpio_pin_set(DISPLAY_RE);
    delay(500);
    spi_deselect(DISPLAY_SPI);    
}

int st7735_init(int orientation) {
    int spi = DISPLAY_SPI;
    
    switch (orientation) {
        case 0: orientation = st7735_orientation_v0; break;
        case 1: orientation = st7735_orientation_v1; break;
        case 2: orientation = st7735_orientation_h0; break;
        case 3: orientation = st7735_orientation_h1; break;
    }
    
    // Init spi port
    if (spi_init(spi) != 0) {
        syslog(LOG_ERR, "tft cannot open spi%u port", spi);
        return 0;
    }
    
    spi_set_cspin(spi, DISPLAY_CS);
    spi_set_speed(spi, DISPLAY_SPEED);

#if !PLATFORM_ESP32
    spi_set(spi, PIC32_SPICON_CKE);
#endif

    // Init re pin
    gpio_disable_analog(DISPLAY_RE);
    gpio_pin_output(DISPLAY_RE);
    gpio_pin_clr(DISPLAY_RE);

    // Init rs pin
    gpio_disable_analog(DISPLAY_RS);
    gpio_pin_output(DISPLAY_RS);
    gpio_pin_clr(DISPLAY_RS);

    syslog(LOG_INFO,
           "tft is at %s, pin cs=%s%d, speed %d Mhz",
           spi_name(spi), spi_csname(spi), spi_cspin(spi),
           spi_get_speed(spi) / 1000
    );
    
    st7735_reset();
    
    st7735_command(st7735_slpout); // Sleep Out

    st7735_command(st7735_colmod); // Interface Pixel Format
    st7735_data(0x05);
   
    st7735_command(st7735_gamset); // Gamma Set
    st7735_data(0x04);

    st7735_command(st7735_gmctrp1); // Gamma (‘+’polarity) Correction Characteristics Setting
    st7735_data(0x3F);
    st7735_data(0x25);
    st7735_data(0x1C);
    st7735_data(0x1E);
    st7735_data(0x20);
    st7735_data(0x12);
    st7735_data(0x2A);
    st7735_data(0x90);
    st7735_data(0x24);
    st7735_data(0x11);
    st7735_data(0x00);
    st7735_data(0x00);
    st7735_data(0x00);
    st7735_data(0x00);
    st7735_data(0x00);

    st7735_command(st7735_gmctrn1); // Gamma (‘-’polarity) Correction Characteristics Setting
    st7735_data(0x20);
    st7735_data(0x20);
    st7735_data(0x20);
    st7735_data(0x20);
    st7735_data(0x05);
    st7735_data(0x00);
    st7735_data(0x15);
    st7735_data(0xA7);
    st7735_data(0x3D);
    st7735_data(0x18);
    st7735_data(0x25);
    st7735_data(0x2A);
    st7735_data(0x2B);
    st7735_data(0x2B);
    st7735_data(0x3A);

    st7735_command(st7735_frmctr1); // Frame Rate Control (In normal mode/ Full colors)
    st7735_data(0x08);
    st7735_data(0x08);

    st7735_command(st7735_invctr); // Display Inversion Control
    st7735_data(0x07);

    st7735_command(st7735_pwctr1); // Power Control
    st7735_data(0x0A);
    st7735_data(0x02);
      
    st7735_command(st7735_pwctr2); // Power Control
    st7735_data(0x02);

    st7735_command(st7735_vmctr1); // VCOM Control 1
    st7735_data(0x50);
    st7735_data(0x5B);

    st7735_command(st7735_vmofctr); // VCOM Offset Control
    st7735_data(0x40);

    if ((orientation == st7735_orientation_v0) || (orientation == st7735_orientation_v1)) {
        tftwidth  = 128;
        tftheight = 160;

        st7735_command(st7735_caset); // Column Address Set
        st7735_data(0x00);
        st7735_data(0x00);
        st7735_data(0x00);
        st7735_data(0x7F);

        st7735_command(st7735_raset); // Row Address Set
        st7735_data(0x00);
        st7735_data(0x00);
        st7735_data(0x00);
        st7735_data(0x9F);
    } else {    
        tftwidth  = 160;
        tftheight = 128;

        st7735_command(st7735_caset); // Column Address Set
        st7735_data(0x00);
        st7735_data(0x00);
        st7735_data(0x00);
        st7735_data(0x9F);
   
        st7735_command(st7735_raset); // Row Address Set
        st7735_data(0x00);
        st7735_data(0x00);
        st7735_data(0x00);
        st7735_data(0x7F);
    }

    st7735_command(st7735_madctl); // Memory Data Access Control
    st7735_data(orientation);

    st7735_command(st7735_dispon); // Display On
    
    return 0;
}

void st7735_scroll(int steps) {
    st7735_command(st7735_vscsad); // Vertical Scroll Start Address of RAM
    st7735_data(0x00);
    st7735_data(tftheight - steps);
}

void st7735_addr_window(int x0, int y0, int x1, int y1) {
    if ((x0 > tftwidth) || (y0 > tftheight) || (x1 > tftwidth) || (y1 > tftheight)) {
        return;
    }
    
    st7735_command(st7735_caset); // Column Address Set
    st7735_data(0x00);
    st7735_data(x0);
    st7735_data(0x00);
    st7735_data(x1);

    st7735_command(st7735_raset); // Row Address Set
    st7735_data(0x00);
    st7735_data(y0);
    st7735_data(0x00);
    st7735_data(y1);

    st7735_command(st7735_ramwr); // Memory Write
}

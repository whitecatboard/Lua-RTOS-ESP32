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

#ifndef ST7735S_H
#define	ST7735S_H

#define st7735_orientation_v0   192
#define st7735_orientation_v1   0
#define st7735_orientation_h0   96
#define st7735_orientation_h1   160

#define st7735_nop 		 0x00 // No Operation
#define st7735_swreset 	 0x01 // Software Reset
#define st7735_rddid 	         0x04 // Read Display ID
#define st7735_rddst 	         0x09 // Read Display Status
#define st7735_rddpm 	         0x0A // Read Display Power Mode
#define st7735_rddmadctl        0x0B // Read Display MADCTL
#define st7735_rddcolmod        0x0C // Read Display Pixel Format
#define st7735_rddim 	         0x0D // Read Display Image Mode
#define st7735_rddsm 	         0x0E // Read Display Signal Mode
#define st7735_rddsdr 	         0x0F // Read Display Self-Diagnostic Result
#define st7735_slpin 	         0x10 // Sleep In
#define st7735_slpout 	         0x11 // Sleep Out
#define st7735_ptlon 	         0x12 // Partial Display Mode On
#define st7735_noron 	         0x13 // Normal Display Mode On
#define st7735_invoff 	         0x20 // Display Inversion Off
#define st7735_invon 	         0x21 // Display Inversion On
#define st7735_gamset 	         0x26 // Gamma Set
#define st7735_dispof           0x28 // Display Off
#define st7735_dispon 	         0x29 // Display On
#define st7735_caset 	         0x2A // Column Address Set
#define st7735_raset 	         0x2B // Row Address Set
#define st7735_ramwr 	         0x2C // Memory Write
#define st7735_rgbset 	         0x2D // Color Setting for 4K, 65K and 262K
#define st7735_ramrd 	         0x2E // Memory Read
#define st7735_ptlar 	         0x30 // Partial Area
#define st7735_scrlar 	         0x33 // Scroll Area Set
#define st7735_teoff 	         0x34 // Tearing Effect Line OFF
#define st7735_teon 	         0x35 // Tearing Effect Line ON
#define st7735_madctl 	         0x36 // Memory Data Access Control
#define st7735_vscsad 	         0x37 // Vertical Scroll Start Address of RAM
#define st7735_idmoff 	         0x38 // Idle Mode Off
#define st7735_idmon 	         0x39 // Idle Mode On
#define st7735_colmod 	         0x3A // Interface Pixel Format
#define st7735_rdid1 	         0xDA // Read ID1 Value
#define st7735_rdid2 	         0xDB // Read ID2 Value
#define st7735_rdid3 	         0xDC // Read ID3 Value
#define st7735_frmctr1 	 0xB1 // Frame Rate Control (In normal mode/ Full colors)
#define st7735_frmctr2 	 0xB2 // Frame Rate Control (In Idle mode/ 8-colors)
#define st7735_frmctr3 	 0xB3 // Frame Rate Control (In Partial mode/ full colors)
#define st7735_invctr 	         0xB4 // Display Inversion Control
#define st7735_pwctr1 	         0xC0 // Power Control
#define st7735_pwctr2 	         0xC1 // Power Control
#define st7735_pwctr3 	         0xC2 // Power Control (In Normal mode/ Full colors)
#define st7735_pwctr4 	         0xC3 // Power Control (In Idle mode/ 8-colors)
#define st7735_pwctr5 	         0xC4 // Power Control (In Partial mode/ full-colors)
#define st7735_vmctr1 	         0xC5 // VCOM Control 1
#define st7735_vmofctr 	 0xC7 // VCOM Offset Control
#define st7735_wrid2 	         0xD1 // Write ID2 Value
#define st7735_wrid3 	         0xD2 // Write ID3 Value
#define st7735_nvfctr1 	 0xD9 // NVM Control Status
#define st7735_nvfctr2 	 0xDE // NVM Read Command
#define st7735_nvfctr3 	 0xDF // NVM Write Command
#define st7735_gmctrp1 	 0xE0 // Gamma (‘+’polarity) Correction Characteristics Setting
#define st7735_gmctrn1 	 0xE1 // Gamma (‘-’polarity) Correction Characteristics Setting
#define st7735_gcv		 0xFC // Gate Pump Clock Frequency Variable
        

int st7735_init(int orientation);
int st7735_width();
int st7735_height();
void st7735_sendcolor(int color);
void st7735_begindata();
void st7735_enddata();
void st7735_addr_window(int x0, int y0, int x1, int y1);

#endif	/* ST7735S_H */


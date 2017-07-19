/*
 * Lua RTOS, graphic display
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L
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

#ifndef GDISPLAY_H
#define	GDISPLAY_H

#include <stdint.h>
#include <math.h>
#include <string.h>

#include <gdisplay/primitives/primitives.h>
#include <gdisplay/fonts/font.h>
#include <gdisplay/image/image.h>

#include <sys/driver.h>

#include <drivers/gdisplay.h>
#include <drivers/ili9341.h>
#include <drivers/st7735.h>
#include <drivers/pcd8544.h>

/*
 * ST7735
 */

// Default types (1.8")
#define CHIPSET_ST7735  1
#define CHIPSET_ST7735B 2
#define CHIPSET_ST7735G 3

// 1.8" types
#define CHIPSET_ST7735_18  CHIPSET_ST7735
#define CHIPSET_ST7735B_18 CHIPSET_ST7735B
#define CHIPSET_ST7735G_18 CHIPSET_ST7735G

// 1.44" types
#define CHIPSET_ST7735G_144 4

// 0.96" types
#define CHIPSET_ST7735_096 5

#define CHIPSET_ST7735_VARIANT_OFFSET CHIPSET_ST7735

/*
 * ILI9341
 */

#define CHIPSET_ILI9341 6

// Color definitions
#define GDISPLAY_BLACK       0x0000      /*   0,   0,   0 */
#define GDISPLAY_NAVY        0x000F      /*   0,   0, 128 */
#define GDISPLAY_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define GDISPLAY_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define GDISPLAY_MAROON      0x7800      /* 128,   0,   0 */
#define GDISPLAY_PURPLE      0x780F      /* 128,   0, 128 */
#define GDISPLAY_OLIVE       0x7BE0      /* 128, 128,   0 */
#define GDISPLAY_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define GDISPLAY_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define GDISPLAY_BLUE        0x001F      /*   0,   0, 255 */
#define GDISPLAY_GREEN       0x07E0      /*   0, 255,   0 */
#define GDISPLAY_CYAN        0x07FF      /*   0, 255, 255 */
#define GDISPLAY_RED         0xF800      /* 255,   0,   0 */
#define GDISPLAY_MAGENTA     0xF81F      /* 255,   0, 255 */
#define GDISPLAY_YELLOW      0xFFE0      /* 255, 255,   0 */
#define GDISPLAY_WHITE       0xFFFF      /* 255, 255, 255 */
#define GDISPLAY_ORANGE      0xFD20      /* 255, 165,   0 */
#define GDISPLAY_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define GDISPLAY_PINK        0xF81F

// Orientation
#define PORTRAIT	0
#define LANDSCAPE	1
#define PORTRAIT_FLIP	2
#define LANDSCAPE_FLIP	3

// Special positions
#define LASTX	-1
#define LASTY	-2
#define CENTER	-3
#define RIGHT	-4
#define BOTTOM	-4

// Fonts
#define DEFAULT_FONT	0
#define DEJAVU18_FONT	1
#define DEJAVU24_FONT	2
#define UBUNTU16_FONT	3
#define COMIC24_FONT	4
#define MINYA24_FONT	5
#define TOONEY32_FONT	6
#define FONT_7SEG		7
#define USER_FONT		8

#define CHIPSET_ILI9341 6
#define CHIPSET_PCD8544 7




#define DEG_TO_RAD 0.01745329252
#define RAD_TO_DEG 57.295779513
#define deg_to_rad 0.01745329252 + 3.14159265359

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#if !defined(max)
#define max(A,B) ( (A) > (B) ? (A):(B))
#endif
#if !defined(min)
#define min(A,B) ( (A) < (B) ? (A):(B))
#endif

#define swap(a, b) { int16_t t = a; a = b; b = t; }

#define LASTX	-1
#define LASTY	-2
#define CENTER	-3
#define RIGHT	-4
#define BOTTOM	-4

typedef struct {
	uint8_t chipset; // Chipset
	driver_error_t *(*init)(uint8_t, uint8_t);
} gdisplay_t;

//  Driver errors
#define  GDISPLAY_ERR_INVALID_CHIPSET             (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  0)
#define  GDISPLAY_ERR_INVALID_COLOR            	  (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  1)
#define  GDISPLAY_ERR_IS_NOT_SETUP            	  (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  2)
#define  GDISPLAY_ERR_COLOR_REQUIRED              (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  3)
#define  GDISPLAY_ERR_POINT_REQUIRED              (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  4)
#define  GDISPLAY_ERR_NOT_ENOUGH_MEMORY           (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  5)
#define  GDISPLAY_ERR_INVALID_RADIUS              (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  6)
#define  GDISPLAY_ERR_INVALID_FONT                (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  7)
#define  GDISPLAY_ERR_IMAGE                		  (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  8)
#define  GDISPLAY_ERR_OUT_OFF_SCREEN              (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  9)
#define  GDISPLAY_ERR_INVALID_ORIENTATION         (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  10)
#define  GDISPLAY_ERR_MISSING_FILE_NAME           (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  11)
#define  GDISPLAY_ERR_IMG_PROCESSING_ERROR        (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  12)
#define  GDISPLAY_ERR_BOOLEAN_REQUIRED            (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  13)
#define  GDISPLAY_ERR_TOUCH_NOT_SUPPORTED         (DRIVER_EXCEPTION_BASE(GDISPLAY_DRIVER_ID) |  14)

driver_error_t *gdisplay_init(uint8_t chipset, uint8_t orientation, uint8_t buffered);
uint8_t gdisplay_is_init();
void gdisplay_begin();
void gdisplay_end();

driver_error_t *gdisplay_get_pixel(int x, int y, uint32_t *color);
driver_error_t *gdisplay_set_pixel(int x, int y, uint32_t color);
driver_error_t *gdisplay_set_font(uint8_t font, const char *file);
driver_error_t *gdisplay_write_char(int x, int y, char c);
driver_error_t *gdisplay_write(int x, int y, const char *str);
driver_error_t *gdisplay_height(int *aheight);
driver_error_t *gdisplay_width(int *awidth);
driver_error_t *gdisplay_hsb(float _hue, float _sat, float _brightness, uint32_t *color) ;
driver_error_t *gdisplay_clear(uint32_t color);
driver_error_t *gdisplay_stroke(uint32_t color);
driver_error_t *gdisplay_foreground(uint32_t color);
driver_error_t *gdisplay_background(uint32_t color);
driver_error_t *gdisplay_on();
driver_error_t *gdisplay_off();
driver_error_t *gdisplay_get_clip_window(int *x0, int *y0, int *x1, int *y1);
driver_error_t *gdisplay_set_clip_window(int x0, int y0, int x1, int y1);
driver_error_t *gdisplay_reset_clip_window();
driver_error_t *gdisplay_type(int8_t *dtype);
driver_error_t *gdisplay_set_rotation(uint16_t newrot);
driver_error_t *gdisplay_set_orientation(uint16_t orient);

uint8_t gdisplay_is_init();
void gdisplay_begin();
void gdisplay_end();
void gdisplay_set_transparency(uint8_t ntransparent);
uint8_t gdisplay_get_transparency();
void gdisplay_set_wrap(uint8_t nwrap);
uint8_t gdisplay_get_wrap();
int gdisplay_get_offset();
void gdisplay_set_offset(int offset);
uint16_t gdisplay_get_rotation();
void gdisplay_set_cursor(int x, int y);
void gdisplay_get_cursor(int *x, int *y);
float gdisplay_get_max_angle();
float gdisplay_get_angle_offset();
driver_error_t *gdisplay_invert(uint8_t on);
void gdisplay_set_angle_offset(float noffset);
void gdisplay_set_force_fixed(uint8_t fixed);
uint8_t gdisplay_get_force_fixed();
void gdisplay_set_bitmap_pixel(int x, int y, uint32_t color, uint8_t *buff, int buffw, int buffh);
uint32_t gdisplay_get_bitmap_pixel(int x, int y, uint8_t *buff, int buffw, int buffh);
void gdisplay_set_bitmap(int x, int y, uint8_t *buff, int buffw, int buffh);
void gdisplay_get_bitmap(int x, int y, uint8_t *buff, int buffw, int buffh);
driver_error_t *gdisplay_color_to_rgb(uint32_t color, int *r, int *g, int *b);
driver_error_t *gdisplay_rgb_to_color(int r, int g, int b, uint32_t *color);
driver_error_t *gdisplay_touch_get(int *x, int *y, int *z);
driver_error_t *gdisplay_touch_get_raw(int *x, int *y, int *z);
driver_error_t *gdisplay_touch_set_cal(int x, int y);

driver_error_t *gdisplay_lock();
driver_error_t *gdisplay_unlock();

#endif	/* GDISPLAY_H */

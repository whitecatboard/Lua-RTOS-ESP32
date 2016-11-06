/*
 * Whitecat, display management functions
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

#ifndef DISPLAY_H
#define	DISPLAY_H

#include <stdint.h>

#include <sys/drivers/display/font.h>

struct display_ops {  
    int  (*init)(int orientation);
    int  (*width)();
    int  (*height)();
    void (*color)(int color);
    void (*begindata)();
    void (*enddata)();
    void (*addr)(int x1, int y1, int x2, int y2);
};

struct display {
    const char   *chipset; // Chipset name
    unsigned char r_depth; // Red depth     
    unsigned char g_depth; // Green depth
    unsigned char b_depth; // Blue depth

    // Chipset specific operations
    struct display_ops ops;
};

const struct display *display_get(const char *chipset);
int display_rgb(int r, int g, int b);
void display_update();

int display_init(const struct display *ddisplay, int orientation, int framed);
void display_fill(int  color);
void display_text(int x, int y, struct font *font, const char *str);
void display_pixel(int x, int y, int color);
void display_framed();
void display_noframed();
void display_xy(int x, int y);
void display_getxy(int *x, int *y);
void display_font(struct font *font);
void display_font_size(int size);
void display_foreground(int color);
void display_background(int color);
void display_clear(int color);
void display_fill_rect(int x0, int y0, int w, int h, int color);
void display_stroke(int color);
void display_hline(int x0, int y0, int w);    
void display_vline(int x0, int y0, int h);
void display_line(int x0, int y0, int x1, int y1);
void display_image(int x0, int y0, uint16_t *image, int w, int h);

#endif	/* DISPLAY_H */


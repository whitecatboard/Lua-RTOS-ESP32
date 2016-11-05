/*
 * Lua RTOS, display management functions
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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/drivers/display/ST7735S/ST7735S.h>
#include <sys/drivers/display.h>
#include <sys/drivers/display/font.h>

static int framed;

static uint16_t *frame_buff = NULL;

static int cursorx, cursory;
static int cfontsize;

// Frame buffer bounding box (must be update)
static int bbx1,bbx2,bby1,bby2;

const struct display displaydevs[] = {
    {
        "st7735", 5, 6, 5,
        {
        	st7735_init,st7735_width,st7735_height,
			st7735_sendcolor, st7735_begindata, st7735_enddata,
			st7735_addr_window
        }
    },
    {NULL}
};

static const struct display *cdisplay;
static struct font *cfont;
static int cforeground;
static int cbackground;
static int cstroke;

static char *display_escape_seq(char *c) {
    int height = (*cdisplay->ops.height)();
    int width  = (*cdisplay->ops.width)();

    if (*c == '[') {
        if ((*(c + 1) == '1') && (*(c + 2) == 'K')) {
            // Erase start of line
            display_fill_rect(0, cursory, cursorx, cfont->header.height, cbackground);
            c = c + 3;
            return c;
        }

        if ((*(c + 1) == '2') && (*(c + 2) == 'K')) {
            // Erase line
            display_fill_rect(0, cursory, width, cfont->header.height, cbackground);
            c = c + 3;
            return c;
        }

        if ((*(c + 1) == '2') && (*(c + 2) == 'J')) {
            // Clear screen
            display_clear(cbackground);
            c = c + 3;
            return c;
        }

        if ((*(c + 1) == '1') && (*(c + 2) == 'J')) {
            // Erase up
            display_fill_rect(0, 0, width, cursory, cbackground);
            c = c + 3;
            return c;
        }

        if ((*(c + 1) == 'K')) {
            // Erase end of line
            display_fill_rect(cursorx, cursory, width - cursorx, cfont->header.height, cbackground);
            c = c + 2;
            return c;
        }

        if ((*(c + 1) == 'J')) {
            // Erase down
            display_fill_rect(0, cursory + cfont->header.height, width, height - cursory - cfont->header.height, cbackground);
            c = c + 2;
            return c;
        }
        
        if ((*(c + 1) == 'A')) {
            // Cursor UP
            c = c + 2;
            return c;
        }

        if ((*(c + 1) == 'B')) {
            // Cursor DOWN
            c = c + 2;
            return c;
        }

        if ((*(c + 1) == 'C')) {
            // Cursor FORWARD
            c = c + 2;
            return c;

        }
        if ((*(c + 1) == 'D')) {
            // Cursor BACKWARD
            c = c + 2;
            return c;
        }
    }
    
    c++;
    return c;
}

static char *display_process_special_chars(char *c) {
    switch (*c) {
        case '\r':
            cursorx = 0;
            return ++c;
            
        case '\n':
            cursorx = 0;
            cursory = cursory + cfont->header.height + 1;
            return ++c;
            
        case '\033':
        case '\27':
            return display_escape_seq(++c);
    } 
    
    return c;
}


static void display_bb_update(int x, int y, int w, int h) {
    w--;
    h--;
    
    if (x < bbx1) bbx1 = x;
    if (x + w > bbx2) bbx2 = x + w;
    if (y < bby1) bby1 = y;
    if (y + h > bby2) bby2 = y + h;    
}

static void display_bb_init() {
    bbx1 = 5000;
    bbx2 = -1;
    bby1 = 5000;
    bby2 = -1;
}

const struct display *display_get(const char *chipset) {
    const struct display *cdisplay;
    
    cdisplay = &displaydevs[0];
    while (cdisplay) {
        if (strcmp(cdisplay->chipset, chipset) == 0) {
            return cdisplay;
        }
        
        cdisplay++;
    }
    
    errno = EINVAL;
    return NULL;
}

// Code a color expressed in it's RGB components into a word
// Makes color conversion to display capabilities
int display_rgb(int r, int g, int b) {
    return (
        ((r >> (8 - cdisplay->r_depth)) << (cdisplay->g_depth + cdisplay->b_depth)) |
        ((g >> (8 - cdisplay->g_depth)) << (cdisplay->b_depth)) |
        (b >> (8 - cdisplay->b_depth))
    );
}

// Update display
void display_update() {
    register uint16_t *start;
    register uint16_t *pixel;
    register int width, offset;
    int x, y;
    
    width = (*cdisplay->ops.width)();

    (*cdisplay->ops.addr)(bbx1, bby1, bbx2, bby2);
    
    (*cdisplay->ops.begindata)();
    
    start = frame_buff + bbx1;
    offset = bby1 * width;
    for(y = bby1;y <= bby2;y++, offset += width) {
        pixel = start + offset;
        for(x = bbx1;x <= bbx2;x++) {
            (*cdisplay->ops.color)(*pixel++);
        }  
    }

    (*cdisplay->ops.enddata)();

    display_bb_init();
}

// Init display
int display_init(const struct display *ddisplay, int orientation, int setframed) {
    int res = 0;
    int width, height;
    
    cdisplay = ddisplay;
    cfontsize = 1;
    
    cursorx = 0;
    cursory = 0;

    cforeground = display_rgb(255, 255, 255);
    cbackground = display_rgb(0, 0, 0);
    cstroke = cforeground;
    
    width = (*cdisplay->ops.width)();
    height = (*cdisplay->ops.height)();
    
    framed = 0;
    if (setframed) {
        framed = 1;

        // Allocate space from frame buffer
        if (frame_buff) {
            free(frame_buff);
        }

        frame_buff = (void *)malloc(2 * width * height);
        if (!frame_buff) {
            errno = ENOMEM;
            return -1;
        }        

        bzero(frame_buff, 2 * width * height);

        display_bb_init();        
    }
    
    res = (*cdisplay->ops.init)(orientation);
    if (!res) {
        return -1;
    }
    
    return 0;
}

void display_fill_rect(int x0, int y0, int w, int h, int color) {
    uint16_t *pixel;
    int x, y, width;
    
    if (framed) {
        width = (*cdisplay->ops.width)();

        display_bb_update(x0, y0, x0 + w - 1, y0 + h - 1);

        for(y=y0;y < y0 + h;y++) {
            pixel = frame_buff + x0 + y * width;
            for(x=x0;x< x0 + w;x++) {
                *pixel++ = color;
            }  
        }

        display_update(cdisplay);
    } else {
        (*cdisplay->ops.addr)(x0, y0, x0 + w - 1, y0 + h - 1);

        (*cdisplay->ops.begindata)();

        for(y=y0;y < y0 + h;y++) {
            for(x=x0;x< x0 + w;x++) {
                (*cdisplay->ops.color)(color);
            }  
        }        

        (*cdisplay->ops.enddata)();
    }    
}


void display_hline(int x0, int y0, int w) {    
    uint16_t *pixel;
    int x, height;;

    if (framed) {
        height = (*cdisplay->ops.height)();

        pixel = frame_buff + y0 * height + x0;
        for(x=x0;x< x0 + w;x++) {
            *pixel++ = cstroke;
        }
    } else {
        (*cdisplay->ops.addr)(x0, y0, x0 + w, y0);

        (*cdisplay->ops.begindata)();

        for(x=x0;x< x0 + w;x++) {
            (*cdisplay->ops.color)(cstroke);
        }  

        (*cdisplay->ops.enddata)();        
    }
}

void display_vline(int x0, int y0, int h) {
    uint16_t *pixel;
    int y, height;

    if (framed) {
        height = (*cdisplay->ops.height)();

        pixel = frame_buff + y0 * height + x0;
         for(y=y0;y< y0 + h;y++) {
            *pixel++ = cstroke;
        }
    } else {
        (*cdisplay->ops.addr)(x0, y0, x0, y0 + h);

        (*cdisplay->ops.begindata)();

        for(y=y0;y< y0 + h;y++) {
            (*cdisplay->ops.color)(cstroke);
        }  

        (*cdisplay->ops.enddata)();        
    }
}


void display_line(int x0, int y0, int x1, int y1) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int two_ds = dx + dx;
    int two_dy = dy + dy;
    int currentx = x0, currenty = y0;
    int xinc = 1, yinc = 1;
    int two_ds_accumulated_error, two_dy_accumulated_error;
    
    if (dx < 0) {
        xinc = -1;
        dx = -dx;
        two_ds = -two_ds;
    }
    
    if (dy < 0) {
        yinc = -1;
        dy = -dy;
        two_dy = -two_dy;
    }
    
    display_pixel(x0, y0, cstroke);

    if ((dx != 0) || (dy != 0)) {        
        if (dy <= dx) {        
            two_ds_accumulated_error = 0;
        
            while (currentx != x1) {
                currentx = currentx + xinc;
                two_ds_accumulated_error = two_ds_accumulated_error + two_dy;

                if (two_ds_accumulated_error > dx) {
                    currenty = currenty + yinc;
                    two_ds_accumulated_error = two_ds_accumulated_error - two_ds;
                }

                display_pixel(currentx, currenty, cstroke);
            }
        } else {
            two_dy_accumulated_error = 0;
            
            while (currenty != y1) {                
                currenty = currenty + yinc;
                two_dy_accumulated_error = two_dy_accumulated_error + two_ds;
                
                if (two_dy_accumulated_error > dy) {                    
                    currentx = currentx + xinc;
                    two_dy_accumulated_error = two_dy_accumulated_error - two_dy;                
                }
                
                display_pixel(currentx, currenty, cstroke);
            }
        }
    }
}


void display_image(int x0, int y0, uint16_t *image, int w, int h) {
    uint16_t *pixel;
    int x, y, width;
    
    width = (*cdisplay->ops.width)();

    if (framed) {
        display_bb_update(x0, y0, w, h);

        for(y=y0;y < y0 + h;y++) {
            pixel = frame_buff + x0 + y * width;
            for(x=x0;x< x0 + w;x++) {
                *pixel++ = *image++;
            }  
        }

        display_update(cdisplay);
    } else {
        (*cdisplay->ops.addr)(x0, y0, w - 1, h - 1);

        (*cdisplay->ops.begindata)();

        for(y=y0;y < y0 + h;y++) {
            for(x=x0;x< x0 + w;x++) {
                (*cdisplay->ops.color)(*image++);
            }  
        }        

        (*cdisplay->ops.enddata)();
    }
}

// Check if current char can be drawed un current cursor position, according
// to display dimensions. If not, update cursor, and makes a vertical scroll
static int check_dimensions(struct font *font, const char *c) {
    int width = (*cdisplay->ops.width)();
    int height = (*cdisplay->ops.height)();

    // Tes if we have enougth space for drawing this char on the screen
    if (cursorx + font->header.width * cfontsize - 1 + 1 >= width) {
        cursorx = 0;
        cursory = cursory + font->header.height * cfontsize;

        while (*c && (*++c == ' '));
    }

    if (cursory >= height) {
        return 0;
/*
        //TO DO: scroll
        cursory = height - font->header.height * cfontsize;
*/
    }
    
    return 1;
}

void display_text(int x, int y, struct font *font, const char *str) {
    char *c;          // Current str char
    char *char_def;   // Definition of current char
    int mask;         // Mask for test current font data pixel
    uint16_t *pixel;
    int i, j, color;
    int width;
    
    if ((x >= 0) && (y >= 0)) {
        cursorx = x;
        cursory = y;
    }
    
    if (!font) font = cfont;
        
    width = (*cdisplay->ops.width)();

    // parse string 
    c = (char *)str;
    while (*c) {
        c = display_process_special_chars(c);
        if (!*c) break;
        
        if (!check_dimensions(font, c)) {
            c++;
            continue;
        }
        
        char_def = font_char_def(font, c);
        
        // Draw current char
        for(i=0; i < font->header.height; i++) {
            mask = 0x80;

            // Draw a pixel for each bit of current line
            for(j=0; j < font->header.width; j++) { 
                if (char_def[i] & mask) {
                    color = cforeground;
                } else {
                    color = cbackground;
                }

                if (framed) {
                    if (cfontsize > 1) {
                        display_fill_rect(cursorx, cursory, cfontsize, cfontsize, color);
                    } else {
                        pixel = frame_buff + cursory * width + cursorx;
                        *pixel = color;
                    }
                } else {
                    if (cfontsize > 1) {
                        display_fill_rect(cursorx, cursory, cfontsize, cfontsize, color);
                    } else {
                        (*cdisplay->ops.addr)(cursorx, cursory, cursorx, cursory);
                        (*cdisplay->ops.begindata)();
                        (*cdisplay->ops.color)(color);
                        (*cdisplay->ops.enddata)();                                                                    
                    }
                }

                mask = (mask >> 1);
                
                // Increment x position
                cursorx = cursorx + cfontsize;
            }
            
            // Prepare for next char line
            cursorx = cursorx - font->header.width * cfontsize;
            cursory = cursory + cfontsize;
        }
        
        free(char_def);
        
        // Prepare for next char
        cursorx = cursorx + font->header.width * cfontsize + 1;
        cursory = cursory - font->header.height * cfontsize;

        c++;
    }
}

void display_pixel(int x, int y, int color) {
    uint16_t *pixel;
    int width;
    
    if (framed) {
        width = (*cdisplay->ops.width)();
        pixel = frame_buff + y * width + x;
        *pixel = color;
    } else {
        (*cdisplay->ops.addr)(x, y, x, y);
        (*cdisplay->ops.begindata)();
        (*cdisplay->ops.color)(color);
        (*cdisplay->ops.enddata)();                                                    
    }
}

void display_clear(int color) {
    int x, y, width, height;
    uint16_t *pixel;
    
    width = (*cdisplay->ops.width)();
    height = (*cdisplay->ops.height)();
    
    if (framed) {
        pixel = frame_buff;

        for(y=0;y < height;y++) {
            for(x=0;x < width;x++) {
                *pixel++ = color;             
            }
        }
    } else {
        (*cdisplay->ops.addr)(0, 0, width - 1, height - 1);
        (*cdisplay->ops.begindata)();

        for(y=0;y < height;y++) {
            for(x=0;x < width;x++) {
                (*cdisplay->ops.color)(color);               
            }
        }
 
        (*cdisplay->ops.enddata)();                                                    
   }
    
    cursorx = 0;
    cursory = 0;
}

void display_framed() {
    framed = 1;
}

void display_noframed() {
    framed = 0;
}

void display_xy(int x, int y) {
    cursorx = x;
    cursory = y;    
}

void display_getxy(int *x, int *y) {
    *x = cursorx;
    *y = cursory;    
}

void display_font(struct font *font) {
    cfont = font;
}

void display_stroke(int color) {
    cstroke = color;
}

void display_foreground(int color) {
    cforeground = color;
}

void display_background(int color) {
    cbackground = color;
}

void display_font_size(int size) {
    cfontsize = size;
}

/*
 * Whitecat, font management functions
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

#ifndef FONT_H
#define	FONT_H

#define MAX_FONT_NAME 20
#define FONT_DEBUG    0

struct font_header {
    unsigned char width;
    unsigned char height;
    unsigned char start;
    unsigned char end;
    unsigned char bpc;
};

struct font {
    char name[20];
    struct font_header header;
    void *data;
};

char *font_char_def(struct font *font, char *c);
struct font *font_load(const char *name);

#endif	/* FONT_H */


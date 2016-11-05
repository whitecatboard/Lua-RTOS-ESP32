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

#include "font.h"

#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/syscalls/mount.h>

char *font_char_def(struct font *font, char *c) {
    int  cidx;  // Char def index
    char *def;  // Char def
    
    // Get position into font data array fot current char
    if ((*c < font->header.start) || (*c > font->header.end)) { 
        cidx = font->header.start;
    } else { 
        cidx = *c - font->header.start;
        cidx = cidx * font->header.bpc;
    }
    
    // Allocate space
    def = (char *)malloc(sizeof(char) * font->header.bpc);
    if (!def) {
        return NULL;
    }
    
#if FONT_DEBUG
    printf("index for %d: %d\n", *c, cidx);
#endif
    
    bcopy((((char *)font->data) + cidx), def, font->header.bpc);
   
#if FONT_DEBUG
    printf("def: ");
    for(i=0;i < font->header.bpc;i++) {
        printf("%02x ", *(def + i));
    }
    printf("\n");
#endif
    
    // Decode char def
/*
    for(i = 0; i < font->header.height; i++) {
        def[i] = 0;
        for(j=0;j < font->header.width;j++) {  
            if ((*(((char *)font->data) + cidx + j) & (1 << i))) {
                def[i] |= (1 << font->header.width - 1 - j);
            }
        }        
    }
*/
    
    return def;
}

struct font *font_load(const char *name) {
    char path[PATH_MAX];
    struct font *font;
    char *npath;
    FILE *fp;
    int res;
    
    // Check font name size
    if (strlen(name) > MAX_FONT_NAME) {
        errno = EINVAL;
        return NULL;
    }
    
    // Get path font
    strcpy(path, "/sys/font/");
    strcat(path, name);
    
    npath = (char *)mount_primary_or_secondary(path);
    if (!npath) {
        errno = ENOMEM;
        return NULL;
    }
            
    // Open font file
    fp = fopen(npath, "r");
    if (!fp) {
        free(npath);
        return NULL;
    }
    
    // Allocate space for font
    font = (struct font*)malloc(sizeof(struct font));
    if (!font) {
        free(npath);
        fclose(fp);
        errno = ENOMEM;
        return NULL;
    }

    strcpy(font->name, name);
    
    // Read header
    res = fread(&font->header,sizeof(struct font_header),1, fp);
    if (res != 1) {
        free(npath);
        fclose(fp);
        free(font);
        return NULL;     
    }
    
    // Allocate space for data
    font->data = (void *)malloc((font->header.end - font->header.start + 1) * font->header.bpc);
    if (!font->data) {
        free(npath);
        fclose(fp);
        free(font);
        errno = ENOMEM;
        return NULL;       
    }
    
    // Read data
    res = fread(font->data,(font->header.end - font->header.start + 1) * font->header.bpc,1, fp);
    if (res != 1) {
        free(npath);
        fclose(fp);
        free(font);
        return NULL;     
    }
    
#if FONT_DEBUG
    printf("width: %d\n",font->header.width);
    printf("height: %d\n",font->header.height);
    printf("bytes per char: %d\n",font->header.bpc);
    printf("start: %d\n",font->header.start);
    printf("end: %d\n",font->header.end);
#endif
    
    free(npath);
    
    // Close
    fclose(fp);
    
    return font;
}

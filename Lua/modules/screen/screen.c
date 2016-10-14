/*
 * Whitecat, Lua screen driver
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

#if LUA_USE_SCREEN

#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "bitmaptypes.h"
#include "endian.h"

#include <errno.h>
#include <drivers/display/display.h>
#include <drivers/display/font.h>

static const struct display *ddisplay = NULL;
static int orientation;
static struct font *font;
static int inited = 0;

static int screen_init(lua_State* L) {
    const char *chipset;

    inited = 0;
    
    chipset = luaL_checkstring(L, 1);
    
    // Get display device structure
    ddisplay = display_get(chipset);
    if (!ddisplay) {
        return luaL_error( L, "unsupported display chipset" );    
    }
    
    // Get orientation
    orientation = luaL_checkinteger(L, 2);    
    
    if ((orientation < 0) || (orientation > 3)) {
        return luaL_error( L, "unsupported orientation" );   
    }
    
    // Init display
    display_init(ddisplay, orientation, 0);
    
    inited = 1;
    
    return 0;
}

static int screen_width(lua_State* L) {
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }
    
    lua_pushinteger(L, (*ddisplay->ops.width)());
    return 1;
}

static int screen_height(lua_State* L) {
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }
    
    lua_pushinteger(L, (*ddisplay->ops.height)());
    return 1;
}

static int screen_rgb(lua_State* L) {
    unsigned int r, g, b;
    
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }
    
    r = luaL_checkinteger(L, 1);
    g = luaL_checkinteger(L, 2);
    b = luaL_checkinteger(L, 3);
    
    if ((r < 0) || (r > 255)) {
        return luaL_error( L, "invalid red component 0 to 255" );   
    }

    if ((g < 0) || (g > 255)) {
        return luaL_error( L, "invalid green component 0 to 255" );   
    }

    if ((b < 0) || (b > 255)) {
        return luaL_error( L, "invalid blue component 0 to 255" );   
    }

    lua_pushinteger(L, display_rgb(r, g, b));
    
    return 1;
}

static int screen_clear(lua_State* L) {
    int color;
    
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    color = luaL_checkinteger(L, 1);

    display_clear(color);
    
    return 0;
}

static int screen_pixel(lua_State* L) {
    int x, y, color;
    
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    x = luaL_checkinteger(L, 1);
    y = luaL_checkinteger(L, 2);
    color = luaL_checkinteger(L, 3);
    
    display_pixel(x, y, color);
    
    return 0;
}

static int screen_hline(lua_State* L) {
    int x, y, w;
    
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    x = luaL_checkinteger(L, 1);
    y = luaL_checkinteger(L, 2);
    w = luaL_checkinteger(L, 3);
    
    display_hline(x, y, w);
    
    return 0;
}

static int screen_vline(lua_State* L) {
    int x, y, h;
    
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    x = luaL_checkinteger(L, 1);
    y = luaL_checkinteger(L, 2);
    h = luaL_checkinteger(L, 3);
    
    display_vline(x, y, h);
    
    return 0;
}

static int screen_line(lua_State* L) {
    int x0, y0, x1, y1;
    
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    x0 = luaL_checkinteger(L, 1);
    y0 = luaL_checkinteger(L, 2);
    x1 = luaL_checkinteger(L, 3);
    y1 = luaL_checkinteger(L, 4);
    
    display_line(x0, y0, x1, y1);
    
    return 0;
}

static int screen_loadimage(lua_State* L) {
    const char *path;
    FILE *fp;
    RGB *rgb;
    int w, h, x, y;
    int res;
    
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    x = luaL_checkinteger(L, 1);
    y = luaL_checkinteger(L, 2);
    path = luaL_checkstring(L, 3);


    fp = fopen(path, "r");
    if (!fp) {
        return luaL_error( L, "%s", strerror(errno));   
    }
    
    res = readSingleImageBMP(fp, &rgb, ddisplay, &w, &h);
    if (res != 0) {
        fclose(fp);
        
        return luaL_error( L, "%s", strerror(errno));   
    }

    fclose(fp);

    display_image(x, y, rgb, w, h);
    free(rgb);
   
    return 0;
}
  
static int screen_font(lua_State* L) {
    const char *name;
    
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    // Check font name
    name = luaL_checkstring(L, 1);
    
    if (strlen(name) > MAX_FONT_NAME) {
        return luaL_error( L, "invalid font name length (max %d)", MAX_FONT_NAME);    
    }
    
    if (font) {    
        // Check if it's loaded
        if (strcmp(font->name, name) == 0) {
            lua_pushboolean(L, 1);
            return 1;
        }
        
        // Not loaded, destroy loaded font
        free(font->data);
        free(font);
    }
    
    // Load font
    font = font_load(name);
    if (!font) {
        return luaL_error( L, "%s", strerror(errno));    
    }
    
    display_font(font);
    
    return 0;
}

static int screen_font_size(lua_State* L) {
    int size;
    
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    if (!font) {
        return luaL_error( L, "please, set a font before" );            
    }

    size = luaL_checkinteger(L, 1);

    display_font_size(size);
    
    return 0;
}

static int screen_foreground(lua_State* L) {
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    display_foreground(luaL_checkinteger(L, 1));
    
    return 0;
}

static int screen_background(lua_State* L) {
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    display_background(luaL_checkinteger(L, 1));
    
    return 0;
}

static int screen_stroke(lua_State* L) {
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    display_stroke(luaL_checkinteger(L, 1));
    
    return 0;
}

static int screen_framed(lua_State* L) {
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    display_framed();
}

static int screen_noframed(lua_State* L) {
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    display_noframed();
}

static int screen_text(lua_State* L) {
    const char *str;
    int x,y;
    
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    if (!font) {
        return luaL_error( L, "please, set a font before" );            
    }
        
    if(lua_isinteger(L, 1)) {
        // Get position
        x = luaL_checkinteger(L, 1);
        y = luaL_checkinteger(L, 2);        

        // Get string
        str = luaL_checkstring(L, 3);        
    } else {
        str = luaL_checkstring(L, 1);   
        
        x = y = -1;
    }
    
    display_text(x, y, font, str);

    return 0;
}

static int screen_xy(lua_State* L) {
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    // Get position
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
     
    display_xy(x, y);
    
    return 0;
}

static int screen_getxy(lua_State* L) {
    int x,y;
    
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    display_getxy(&x, &y);

    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    
    return 2;
}

static int screen_tty_on(lua_State* L) {
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    if (!font) {
        return luaL_error( L, "please, set a font before" );            
    }

    tty_to_display(1);
    
    display_xy(0, 0);
    
    return 0;
}

static int screen_tty_off(lua_State* L) {
    if (!inited) {
        return luaL_error( L, "please, init the screen" );    
    }

    if (!font) {
        return luaL_error( L, "please, set a font before" );            
    }

    tty_to_display(0);

    return 0;
}

static const luaL_Reg screen[] = {
    {"init", screen_init},
    {"width", screen_width},
    {"height", screen_height},
    {"rgb", screen_rgb},
    {"clear", screen_clear},
    {"pixel", screen_pixel},
    {"vline", screen_vline},
    {"hline", screen_hline},
    {"line", screen_line},
    {"loadimage", screen_loadimage},
    {"setfont", screen_font},
/*
    {"setfontsize", screen_font_size},
*/
    {"setforeground", screen_foreground},
    {"setbackground", screen_background},
    {"setstroke", screen_stroke},
    {"framed", screen_framed},
    {"noframed", screen_noframed},
    {"text", screen_text},
    {"setxy", screen_xy},
    {"getxy", screen_getxy},
/*
    {"ttyon", screen_tty_on},
    {"ttyoff", screen_tty_off},
*/
    {NULL, NULL}
};

int luaopen_screen(lua_State* L) {
    luaL_newlib(L, screen);

    lua_pushinteger( L, 0 );
    lua_setfield( L, -2, "OrientationV0" );

    lua_pushinteger( L, 1 );
    lua_setfield( L, -2, "OrientationV1" );

    lua_pushinteger( L, 2 );
    lua_setfield( L, -2, "OrientationH0" );

    lua_pushinteger( L, 3 );
    lua_setfield( L, -2, "OrientationH1" );
    
    return 1;
}

#endif
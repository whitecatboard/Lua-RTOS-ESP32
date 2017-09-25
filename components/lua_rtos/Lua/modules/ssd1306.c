
#include "luartos.h"

//#if CONFIG_LUA_RTOS_LUA_USE_SSD1306

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"

#include "modules.h"

#include "ssd1306.h"


int autoshow = 1;
int inv = 0;

/*
static int lssd1306_init( lua_State* L ) {
    ssd1306_init();
    return 0;
}
*/
static int lssd1306_cls( lua_State* L){
    ssd1306_clear();
	if(autoshow) ssd1306_show();
    return 0;
}
static int lssd1306_show( lua_State* L){
    ssd1306_show();
    return 0;
}
static int lssd1306_autoshow( lua_State* L){
    autoshow = lua_gettop(L) == 0 ? 1 : luaL_checkinteger(L, 1);
    return 0;
}
static int lssd1306_invert( lua_State* L){
	int n = lua_gettop(L);
	int inv;
	if(n > 0) 
		inv = lua_toboolean(L, 1);
	else
		inv = inv == 0 ? 1 : 0;
	ssd1306_invert(inv);
	if(autoshow) ssd1306_show();
	return 0;
}
static int lssd1306_dim( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_dim(
			n > 0 ? lua_toboolean(L, 1) : 0
			);
	return 0;
}
static int lssd1306_startscroll( lua_State* L){
	int n = lua_gettop(L);
	if(n < 1) return 0;
	ssd1306_startscroll(
			luaL_checkinteger(L, 1),
			n > 1 ? luaL_checkinteger(L, 2) : 0,
			n > 2 ? luaL_checkinteger(L, 3) : 128-1
			);
	return 0;
}
static int lssd1306_stopscroll( lua_State* L){
	ssd1306_stopscroll();
	return 0;
}
static int lssd1306_rotation( lua_State* L){
	int n = lua_gettop(L);
	int r = (n > 0) ? luaL_checkinteger(L,1) : 0;
	if(r<0 || r>3) r = 0;
	ssd1306_rotation(r);
	if(autoshow) ssd1306_show();
	return 0;
}

static int lssd1306_point( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_pixel(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0, 
		n>2 ? luaL_checkinteger(L,3) : 1
		);
	if(autoshow) ssd1306_show();
	return 0;
}

static int lssd1306_line( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_line(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0, 
		n>2 ? lua_tonumber(L,3) : 0, 
		n>3 ? lua_tonumber(L,4) : 0, 
		n>4 ? luaL_checkinteger(L,5) : 1
		);
	if(autoshow) ssd1306_show();
	return 0;
}

static int lssd1306_rect( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_rect(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0, 
		n>2 ? lua_tonumber(L,3) : 0, 
		n>3 ? lua_tonumber(L,4) : 0, 
		n>4 ? luaL_checkinteger(L,5) : 1
		);
	if(autoshow) ssd1306_show();
	return 0;
}
static int lssd1306_fillRect( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_rectFill(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0, 
		n>2 ? lua_tonumber(L,3) : 0, 
		n>3 ? lua_tonumber(L,4) : 0, 
		n>4 ? luaL_checkinteger(L,5) : 1
		);
	if(autoshow) ssd1306_show();
	return 0;
}

static int lssd1306_roundRect( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_roundRect(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0, 
		n>2 ? lua_tonumber(L,3) : 0, 
		n>3 ? lua_tonumber(L,4) : 0, 
		n>4 ? lua_tonumber(L,5) : 4, 
		n>5 ? luaL_checkinteger(L,6) : 1
		);
	if(autoshow) ssd1306_show();
	return 0;
}
static int lssd1306_fillRoundRect( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_roundRectFill(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0, 
		n>2 ? lua_tonumber(L,3) : 0, 
		n>3 ? lua_tonumber(L,4) : 0, 
		n>4 ? lua_tonumber(L,5) : 4, 
		n>5 ? luaL_checkinteger(L,6) : 1
		);
	if(autoshow) ssd1306_show();
	return 0;
}

static int lssd1306_circle( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_circle(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0, 
		n>2 ? lua_tonumber(L,3) : 1, 
		n>3 ? luaL_checkinteger(L,4) : 1
		);
	if(autoshow) ssd1306_show();
	return 0;
}
static int lssd1306_fillCircle( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_circleFill(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0, 
		n>2 ? lua_tonumber(L,3) : 1, 
		n>3 ? luaL_checkinteger(L,4) : 1
		);
	if(autoshow) ssd1306_show();
	return 0;
}

static int lssd1306_triangle( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_triangle(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0, 
		n>2 ? lua_tonumber(L,3) : 0, 
		n>3 ? lua_tonumber(L,4) : 0, 
		n>4 ? lua_tonumber(L,5) : 0, 
		n>5 ? lua_tonumber(L,6) : 0, 
		n>6 ? luaL_checkinteger(L,7) : 1
		);
	if(autoshow) ssd1306_show();
	return 0;
}
static int lssd1306_fillTriangle( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_triangleFill(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0, 
		n>2 ? lua_tonumber(L,3) : 0, 
		n>3 ? lua_tonumber(L,4) : 0, 
		n>4 ? lua_tonumber(L,5) : 0, 
		n>5 ? lua_tonumber(L,6) : 0, 
		n>6 ? luaL_checkinteger(L,7) : 1
		);
	if(autoshow) ssd1306_show();
	return 0;
}

static int lssd1306_bitmap( lua_State* L){
	int n = lua_gettop(L);
	if(n < 5) return 0;
	if(!lua_istable(L, 5)) return 0;
	
	int w = lua_tonumber(L,3);
	int h = lua_tonumber(L,4);
	int sz = w*h/8;
	
	if(sz <= 0) return 0;
	
	unsigned char* bmp = (unsigned char*)malloc(sz);
	int i;
	for(i = 0; i < sz; i++){
		lua_rawgeti(L, 5, i+1);
		bmp[i] = (unsigned char)luaL_checkinteger(L, -1);
		lua_pop(L, 1);
	}
	ssd1306_bitmap(
		lua_tonumber(L,1), //x
		lua_tonumber(L,2), //y
		w, //w
		h, //h
		bmp, //*bmp
		n>5 ? luaL_checkinteger(L,6) : 1
		);
	free(bmp);
	if(autoshow) ssd1306_show();
	return 0;
}

static int lssd1306_textSize( lua_State* L){
	int n = lua_gettop(L);
	if(n>0) ssd1306_txtSize(lua_tonumber(L,1));
	return 0;
}
static int lssd1306_textColor( lua_State* L){
	int n = lua_gettop(L);
	if(n>0) ssd1306_txtColor(
						luaL_checkinteger(L,1),
						n>1 ? luaL_checkinteger(L,2) : 0
						);
	return 0;
}
static int lssd1306_textFont( lua_State* L){
	int n = lua_gettop(L);
	//ssd1306_txtFont(int n, bool b, bool i, int sz)
	ssd1306_txtFont(
			n>0 ? luaL_checkinteger(L,1) : 0,
			n>1 ? luaL_checkinteger(L,2) : 0,
			n>2 ? luaL_checkinteger(L,3) : 0,
			n>3 ? luaL_checkinteger(L,4) : 9
			);
	return 0;
}
//void ssd1306_txtGetBnd(char* pstr, int x, int y, int* x1, int* y1, int* w, int* h)
static int lssd1306_textGetBnd( lua_State* L){
	int16_t  x1, y1;
	uint16_t w, h;
	int n = lua_gettop(L);
	if(n < 1) return 0;
	ssd1306_txtGetBnd(
			luaL_checkstring(L,1),
			n>1 ? luaL_checkinteger(L,2) : 0,
			n>2 ? luaL_checkinteger(L,3) : 0,
			&x1, &y1,
			&w, &h
			);
	lua_pushinteger(L, x1);
	lua_pushinteger(L, y1);
	lua_pushinteger(L, w);
	lua_pushinteger(L, h);
	return 4;
}
static int lssd1306_cursor( lua_State* L){
	int n = lua_gettop(L);
	ssd1306_txtCursor(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0
		);
	return 0;
}
static int lssd1306_print( lua_State* L){
	int n = lua_gettop(L);
	int i;
	for(i = 1; i < n; i++){
		ssd1306_print(lua_tostring(L,i) );
	}
	if(n > 0) ssd1306_println( lua_tostring(L,n) );
	if(autoshow) ssd1306_show();
    return 0;
}
static int lssd1306_chr( lua_State* L){
	int n = lua_gettop(L);
	if(n < 2)
		ssd1306_chr( n>0 ? luaL_checkinteger(L,1) : 32 );
	else{
		uint8_t chr = luaL_checkinteger(L,1);
		ssd1306_chrPos(
			n>1 ? lua_tonumber(L,2) : 0, //x,
			n>2 ? lua_tonumber(L,3) : 0, //y, 
			chr, 
			n>3 ? lua_tonumber(L,4) : 1, //clr, 
			n>4 ? lua_tonumber(L,5) : 0, //bg, 
			n>5 ? lua_tonumber(L,6) : 1  //size
			);
	}
	if(autoshow) ssd1306_show();
	return 0;
}

/**
static int lssd1306_cls( lua_State* L){
    ssd1306_clear();

    return 0;
}

static int lssd1306_cls( lua_State* L){
    ssd1306_clear();

    return 0;
}
**/


static const LUA_REG_TYPE lssd1306_map[] = {
//    { LSTRKEY( "init"  ),			LFUNCVAL( lssd1306_init ) },
    { LSTRKEY( "cls" ),				LFUNCVAL( lssd1306_cls ) },
    { LSTRKEY( "show" ),			LFUNCVAL( lssd1306_show ) },
    { LSTRKEY( "autoshow" ),		LFUNCVAL( lssd1306_autoshow ) },
    { LSTRKEY( "invert" ),			LFUNCVAL( lssd1306_invert ) },    
    { LSTRKEY( "dim" ),				LFUNCVAL( lssd1306_dim ) },    
    { LSTRKEY( "scrollStart" ),		LFUNCVAL( lssd1306_startscroll ) },    
    { LSTRKEY( "scrollStop" ),		LFUNCVAL( lssd1306_stopscroll ) },    
    { LSTRKEY( "rotation" ),		LFUNCVAL( lssd1306_rotation ) },    
    
    { LSTRKEY( "point" ),			LFUNCVAL( lssd1306_point ) },
    { LSTRKEY( "line" ),			LFUNCVAL( lssd1306_line ) },
    { LSTRKEY( "rect" ),			LFUNCVAL( lssd1306_rect ) },
    { LSTRKEY( "rectFill" ),		LFUNCVAL( lssd1306_fillRect ) },
    { LSTRKEY( "rectRound" ),		LFUNCVAL( lssd1306_roundRect ) },
    { LSTRKEY( "rectRoundFill" ),	LFUNCVAL( lssd1306_fillRoundRect ) },
    { LSTRKEY( "circle" ),			LFUNCVAL( lssd1306_circle ) },
    { LSTRKEY( "circleFill" ),		LFUNCVAL( lssd1306_fillCircle ) },
    { LSTRKEY( "triangle" ),		LFUNCVAL( lssd1306_triangle ) },
    { LSTRKEY( "triangleFill" ),	LFUNCVAL( lssd1306_fillTriangle ) },
    
    { LSTRKEY( "bitmap" ),			LFUNCVAL( lssd1306_bitmap ) },
    
    { LSTRKEY( "textColor" ),		LFUNCVAL( lssd1306_textColor ) },
    { LSTRKEY( "textSize" ),		LFUNCVAL( lssd1306_textSize ) },
    { LSTRKEY( "textFont" ),		LFUNCVAL( lssd1306_textFont ) },
    { LSTRKEY( "textPos" ),			LFUNCVAL( lssd1306_cursor ) },
    { LSTRKEY( "textGetBounds" ),	LFUNCVAL( lssd1306_textGetBnd ) },
    { LSTRKEY( "print" ),			LFUNCVAL( lssd1306_print ) },
    { LSTRKEY( "chr" ),				LFUNCVAL( lssd1306_chr ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_ssd1306( lua_State *L ) {
	ssd1306_init();
    return 0;
}

MODULE_REGISTER_MAPPED(SSD1306, ssd1306, lssd1306_map, luaopen_ssd1306);

//#endif

/*

ssd1306.cls()
ssd1306.textPos(0, 0)
ssd1306.print("Hello World!!", 11.11)

*/

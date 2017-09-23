
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

static int lssd1306_init( lua_State* L ) {
    ssd1306_init();
    return 0;
}

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
    autoshow =
		lua_gettop(L) == 0 ? 1 : luaL_checkinteger(L, 1);
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

static int lssd1306_textSize( lua_State* L){
	int n = lua_gettop(L);
	
	if(n>0) ssd1306_txtSize(lua_tonumber(L,1));
	return 0;
}
static int lssd1306_textColor( lua_State* L){
	int n = lua_gettop(L);
	
	if(n>0) ssd1306_txtColor(luaL_checkinteger(L,1));
	return 0;
}
static int lssd1306_cursor( lua_State* L){
	int n = lua_gettop(L);
	
	ssd1306_txtCursor(
		n>0 ? lua_tonumber(L,1) : 0, 
		n>1 ? lua_tonumber(L,2) : 0
		);
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
    { LSTRKEY( "init"  ),			LFUNCVAL( lssd1306_init ) },
    { LSTRKEY( "cls" ),				LFUNCVAL( lssd1306_cls ) },
    { LSTRKEY( "show" ),			LFUNCVAL( lssd1306_show ) },
    { LSTRKEY( "print" ),			LFUNCVAL( lssd1306_print ) },
    { LSTRKEY( "point" ),			LFUNCVAL( lssd1306_point ) },
    { LSTRKEY( "textColor" ),			LFUNCVAL( lssd1306_textColor ) },
    { LSTRKEY( "textSize" ),			LFUNCVAL( lssd1306_textSize ) },
    { LSTRKEY( "textPos" ),			LFUNCVAL( lssd1306_cursor ) },
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

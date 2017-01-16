/*
** $Id: linit.c,v 1.38 2015/01/05 13:48:33 roberto Exp $
** Initialization of libraries for lua.c and other clients
** See Copyright Notice in lua.h
*/


#define linit_c
#define LUA_LIB

/*
** If you embed Lua in your program and need to open the standard
** libraries, call luaL_openlibs in your program. If you need a
** different set of libraries, copy this file to your project and edit
** it to suit your needs.
**
** You can also *preload* libraries, so that a later 'require' can
** open the library, which is already linked to the application.
** For that, do the following code:
**
**  luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");
**  lua_pushcfunction(L, luaopen_modname);
**  lua_setfield(L, -2, modname);
**  lua_pop(L, 1);  // remove _PRELOAD table
*/
 
#include "lprefix.h"

#include <stddef.h>
#include <string.h>

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"

/*
** these libs are loaded by lua.c and are readily available to any Lua
** program
*/

#include "modules.h"
#include <sys/debug.h>

extern const luaL_Reg lua_libs1[];

MODULE_REGISTER_UNMAPPED(_G, _G, luaopen_base);
MODULE_REGISTER_UNMAPPED(IO, io, luaopen_io);
MODULE_REGISTER_UNMAPPED(UTF8, utf8, luaopen_utf8);
MODULE_REGISTER_UNMAPPED(PACKAGE, package, luaopen_package);

LUALIB_API void luaL_openlibs (lua_State *L) {
  const luaL_Reg *lib = lua_libs1;

  for (; lib->name; lib++) {
    if (lib->func) {
  		debug_free_mem_begin(luaL_openlibs);

		#if LUA_USE_ROTABLE
		if (luaR_findglobal(lib->name,strlen(lib->name))) {
	        lua_pushcfunction(L, lib->func);
	        lua_pushstring(L, lib->name);
	        lua_call(L, 1, 0);			
		} else {
			luaL_requiref(L, lib->name, lib->func, 1);
			lua_pop(L, 1);  /* remove lib */
		}
		#else
			luaL_requiref(L, lib->name, lib->func, 1);
			lua_pop(L, 1);  /* remove lib */
		#endif
		
	  	debug_free_mem_end(luaL_openlibs, lib->name);
    }
  }
}

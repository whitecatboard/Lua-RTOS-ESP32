// Rename main in lua.c with lua_main
// Remove get_prompt in lua.c
// Rename pushline to luaos_pushline in lua.c

#ifndef WLUA_CONF
#define WLUA_CONF

#include <limits.h>
#include <stdint.h>

#define LUA_OS_VER "beta 0.1"
	
#if LUA_USE_LUA_LOCK
	void LuaLock(lua_State *L);
	void LuaUnlock(lua_State *L);

	#define lua_lock(L)          LuaLock(L)
	#define lua_unlock(L)        LuaUnlock(L)
	#define luai_threadyield(L) {lua_unlock(L); lua_lock(L);}
#else
	#define lua_lock(L)
	#define lua_unlock(L)        
	#define luai_threadyield(L) 
#endif

#undef  LUA_PROMPT
#define LUA_PROMPT		"> "

#undef  LUA_PROMPT2
#define LUA_PROMPT2		">> "

#undef  LUA_MAXINPUT
#define LUA_MAXINPUT 256

#undef  LUA_ROOT
#define LUA_ROOT	"/"

#undef  LUA_LDIR
#define LUA_LDIR	LUA_ROOT "lib/share/lua/"

#undef  LUA_CDIR
#define LUA_CDIR	LUA_ROOT "lib/lua/"

#undef  LUA_COPYRIGHT
#define LUA_COPYRIGHT	"Lua RTOS " LUA_OS_VER " powered by " LUA_RELEASE 

#undef  LUAI_THROW
#define LUAI_THROW(L,c)	longjmp((c)->b, 1)

#undef  LUAI_TRY
#define LUAI_TRY(L,c,a)	if (setjmp((c)->b) == 0) { a }

#undef  luai_jmpbuf
#define luai_jmpbuf jmp_buf

#undef LUA_TMPNAMBUFSIZE
#define LUA_TMPNAMBUFSIZE	32

#if !defined(LUA_TMPNAMTEMPLATE)
#define LUA_TMPNAMTEMPLATE	"/tmp/lua_XXXXXX"
#endif

#define lua_tmpnam(b,e) { \
        strcpy(b, LUA_TMPNAMTEMPLATE); \
        e = mkstemp(b); \
        if (e != -1) close(e); \
        e = (e == -1); }

#undef  lua_readline
#define lua_readline(L,b,p)     ((void)L, (linenoise(b, p)) != -1)

#define lua_saveline(L,idx)     { (void)L; (void)idx; }
#define lua_freeline(L,b)       { (void)L; (void)b; }


#ifdef liolib_c
#undef liolib_c
#include <Lua/modules/liolib_adds.inc>
#endif

#ifdef loslib_c
#undef loslib_c
#include <Lua/modules/loslib_adds.inc>
#endif

#ifdef lua_c
#undef lua_c
static int  report (lua_State *L, int status);
static void l_message (const char *pname, const char *msg);
static int  runargs (lua_State *L, char **argv, int n);
static void print_version (void);
#if LUA_USE_ROTABLE
void doREPL (lua_State *L);
#else
static void doREPL (lua_State *L);
#endif
static void print_usage (const char *badoption);
static int  collectargs (char **argv, int *first);
static void createargtable (lua_State *L, char **argv, int argc, int script);

#include "lauxlib.h"
#include "lualib.h"
#include "modules.h"

#include <Lua/modules/lua_adds.inc>

#endif
			
#endif

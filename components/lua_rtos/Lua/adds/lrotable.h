/* Read-only tables for Lua */

#ifndef lrotable_h
#define lrotable_h

#include "lua.h"
#include "llimits.h"
#include "lobject.h"
#include "luaconf.h"
#include "lrodefs.h"

#include <stdio.h>

// TO DO: ??
#define luaS_newro(L, s)  (luaS_newlstr(L, s, strlen(s)))

// TO DO: ??
#define setnvalue(obj,x) \
  { lua_Number i_x = (x); TValue *i_o=(obj); i_o->value_.n=i_x; i_o->tt_=LUA_TNUMBER; }
  
  
/* Macros one can use to define rotable entries */
#ifndef LUA_PACK_VALUE
#define LRO_FUNCVAL(v)  {{.p = v}, LUA_TLCF}
#define LRO_LUDATA(v)   {{.p = v}, LUA_TLIGHTUSERDATA}
#define LRO_NUMVAL(v)   {{.n = v}, LUA_TNUMFLT}
#define LRO_INTVAL(v)   {{.i = v}, LUA_TNUMINT}
#define LRO_ROVAL(v)    {{.p = (void*)v}, LUA_TROTABLE}
#define LRO_NILVAL      {{.p = NULL}, LUA_TNIL}
#define LRO_STRVAL(v)   {{.p = v}, LUA_TSTRING}
#else // #ifndef LUA_PACK_VALUE
#define LRO_NUMVAL(v)   {.value.n = v}
#define LRO_INTVAL(v)   {.value.i = v}
#ifdef ELUA_ENDIAN_LITTLE
#define LRO_FUNCVAL(v)  {{(int)v, add_sig(LUA_TLCF)}}
#define LRO_LUDATA(v)   {{(int)v, add_sig(LUA_TLIGHTUSERDATA)}}
#define LRO_ROVAL(v)    {{(int)v, add_sig(LUA_TROTABLE)}}
#define LRO_NILVAL      {{0, add_sig(LUA_TNIL)}}
#else // #ifdef ELUA_ENDIAN_LITTLE
#define LRO_FUNCVAL(v)  {{add_sig(LUA_TLCF), (int)v}}
#define LRO_LUDATA(v)   {{add_sig(LUA_TLIGHTUSERDATA), (int)v}}
#define LRO_ROVAL(v)    {{add_sig(LUA_TROTABLE), (int)v}}
#define LRO_NILVAL      {{add_sig(LUA_TNIL), 0}}
#endif // #ifdef ELUA_ENDIAN_LITTLE
#endif // #ifndef LUA_PACK_VALUE

#define LRO_STRKEY(k)   {LUA_TSTRING, {.strkey = k}}
#define LRO_NUMKEY(k)   {LUA_TNUMFLT, {.numkey = k}}
#define LRO_NILKEY      {LUA_TNIL,    {.strkey=NULL}}

/* Maximum length of a rotable name and of a string key*/
#define LUA_MAX_ROTABLE_NAME      32

/* Type of a numeric key in a rotable */
typedef int luaR_numkey;

/* The next structure defines the type of a key */
typedef struct
{
  int type;
  union
  {
    const char*   strkey;
    luaR_numkey   numkey;
  } id;
} luaR_key;

/* An entry in the read only table */
typedef struct
{
  const luaR_key key;
  const TValue value;
} luaR_entry;

const TValue* luaR_findglobal(const char *key, unsigned len);
int luaR_findfunction(lua_State *L, const luaR_entry *ptable);
const TValue* luaR_findentry(const void *pentry, const char *strkey, luaR_numkey numkey, unsigned *ppos);
void luaR_getcstr(char *dest, const TString *src, size_t maxsize);
void luaR_next(lua_State *L, void *data, TValue *key, TValue *val);
int luaR_isrotable(const void *p);
LUA_API void lua_pushrotable (lua_State *L, void *p);
const TValue *luaL_rometatable(const void *data);
int luaH_getn_ro (void *t);
void luaR_next(lua_State *L, void *data, TValue *key, TValue *val);
int luaH_next_ro (lua_State *L, void *t, StkId key);

int luaR_index(lua_State *L, const void *funcs, const void *consts);
int luaR_error(lua_State *L);
LUALIB_API int luaL_newmetarotable (lua_State *L, const char* tname, void *p);

#endif

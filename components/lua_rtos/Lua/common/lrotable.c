/* Read-only tables for Lua */
#define LUAC_CROSS_FILE

#include "luartos.h"

#if LUA_USE_ROTABLE

#include "lua.h"
#include <string.h>
#include "lauxlib.h"
#include "lstring.h"
#include "lapi.h"
#include "lrotable.h"
#include "lobject.h"

/* Local defines */
#define LUAR_FINDFUNCTION     0
#define LUAR_FINDVALUE        1

/* Externally defined read-only table array */
extern const luaR_entry lua_rotable[];

LUA_API void lua_pushrotable (lua_State *L, void *p) {
	lua_lock(L);
    setrvalue(L->top, p);
    api_incr_top(L);
	lua_unlock(L);
}

LUALIB_API int luaL_rometatable (lua_State *L, const char* tname, void *p) {
  lua_getfield(L, LUA_REGISTRYINDEX, tname);  /* get registry.name */
  if (!lua_isnil(L, -1))  /* name already in use? */
    return 0;  /* leave previous value on top, but return 0 */
  lua_pop(L, 1);
  lua_pushrotable(L, p);
  lua_pushvalue(L, -1);
  lua_setfield(L, LUA_REGISTRYINDEX, tname);  /* registry.name = metatable */
  return 1;
}

void luaA_pushobject (lua_State *L, const TValue *o) {
  setobj2s(L, L->top, o);
  api_incr_top(L);
}

/* Find a global "read only table" in the constant lua_rotable array */
const TValue* luaR_findglobal(const char *name, unsigned len) {
  unsigned i;

  if (strlen(name) > LUA_MAX_ROTABLE_NAME)
    return NULL;
  for (i=0; lua_rotable[i].key.id.strkey; i ++)
    if (*lua_rotable[i].key.id.strkey != '\0' && strlen(lua_rotable[i].key.id.strkey) == len && !strncmp(lua_rotable[i].key.id.strkey, name, len)) {
      return &lua_rotable[i].value;
    }
  return NULL;
}

/* Find an entry in a rotable and return it */
static const TValue* luaR_auxfind(const luaR_entry *pentry, const char *strkey, luaR_numkey numkey, unsigned *ppos) {
  const TValue *res = luaO_nilobject;
  unsigned i = 0;

  if (pentry == NULL)
    return luaO_nilobject;
  while(pentry->key.type != LUA_TNIL) {
    if ((strkey && (pentry->key.type == LUA_TSTRING) && (!strcmp(pentry->key.id.strkey, strkey))) ||
        (!strkey && ((pentry->key.type & 0b111) == LUA_TNUMBER) && ((luaR_numkey)pentry->key.id.numkey == numkey))) {
      res = &pentry->value;
      break;
    }
    i ++; pentry ++;
  }

  if (res && ppos)
    *ppos = i;   
  
  return res;
}

int luaR_findfunction(lua_State *L, const luaR_entry *ptable) {
  const TValue *res = NULL;
  const char *key = luaL_checkstring(L, 2);
    
  res = luaR_auxfind(ptable, key, 0, NULL);  
  if (res && ttislcf(res)) {
    luaA_pushobject(L, res);
    return 1;
  }
  else
    return 0;
}

/* Find an entry in a rotable and return its type 
   If "strkey" is not NULL, the function will look for a string key,
   otherwise it will look for a number key */
const TValue* luaR_findentry(const void *pentry, const char *strkey, luaR_numkey numkey, unsigned *ppos) {
	if (pentry) {
		return luaR_auxfind((const luaR_entry *)pentry, strkey, numkey, ppos);						
	} else {
		return luaR_auxfind(lua_rotable, strkey, numkey, ppos);				
	}
}
extern uint32_t _rodata_start;
extern uint32_t _lit4_end;
extern uint32_t _lua_rtos_rodata_start;
extern uint32_t _lua_rtos_rodata_end;

int luaR_isrotable(const void *p) {
	return (
			((&_rodata_start) <= (uint32_t *)p && (uint32_t *)p <= (&_lit4_end)) ||
			((&_lua_rtos_rodata_start) <= (uint32_t *)p && (uint32_t *)p <= (&_lua_rtos_rodata_end))
	);
}

/* Find the metatable of a given table */
void* luaR_getmeta(void *data) {
  const TValue *res = luaR_auxfind((const luaR_entry*)data, "__metatable", 0, NULL);
  return res && ttisrotable(res) ? rvalue(res) : NULL;
}

int luaH_getn_ro (void *t) {
  int i = 1, len=0;
  
  while(luaR_findentry(t, NULL, i ++, NULL))
    len ++;
  return len;
}

static void luaR_next_helper(lua_State *L, const luaR_entry *pentries, int pos, TValue *key, TValue *val) {
  setnilvalue(key);
  setnilvalue(val);
  if (pentries[pos].key.type != LUA_TNIL) {
    /* Found an entry */
    if (pentries[pos].key.type == LUA_TSTRING)
      setsvalue(L, key, luaS_newro(L, pentries[pos].key.id.strkey))
    else
      setnvalue(key, (lua_Number)pentries[pos].key.id.numkey)
   setobj2s(L, val, &pentries[pos].value);
  }
}

/* next (used for iteration) */
void luaR_next(lua_State *L, void *data, TValue *key, TValue *val) {
  const luaR_entry* pentries = (const luaR_entry*)data;
  char strkey[LUA_MAX_ROTABLE_NAME + 1], *pstrkey = NULL;
  luaR_numkey numkey = 0;
  unsigned keypos;
  
  /* Special case: if key is nil, return the first element of the rotable */
  if (ttisnil(key)) 
    luaR_next_helper(L, pentries, 0, key, val);
  else if (ttisstring(key) || ttisnumber(key)) {
    /* Find the previoud key again */  
    if (ttisstring(key)) {
		// TO DO: ??? rawtsvalue changed to tsvalue
      luaR_getcstr(strkey, tsvalue(key), LUA_MAX_ROTABLE_NAME);          
      pstrkey = strkey;
    } else   
      numkey = (luaR_numkey)nvalue(key);
    luaR_findentry(data, pstrkey, numkey, &keypos);
    /* Advance to next key */
    keypos ++;    
    luaR_next_helper(L, pentries, keypos, key, val);
  }
}

/* Convert a Lua string to a C string */
void luaR_getcstr(char *dest, const TString *src, size_t maxsize) {
  // TO DO: ??
  // There are 2 fields shrlen / lnglen
  //src->tsv.len+1 > maxsize
  
  if (src->shrlen+1 > maxsize)
    dest[0] = '\0';
  else {
    memcpy(dest, getstr(src), src->shrlen);
    dest[src->shrlen] = '\0';
  } 
}

int luaH_next_ro (lua_State *L, void *t, StkId key) {
  luaR_next(L, t, key, key+1);
  return ttisnil(key) ? 0 : 1;
}

int luaR_index(lua_State *L, const void *funcs, const void *consts) {
	int res;

	if ((res = luaR_findfunction(L, funcs)) != 0)
		return res;

	if (consts) {
		const char *key = luaL_checkstring(L, 2);
		const TValue *val = luaR_findentry(consts, key, 0, NULL);
		if (val != luaO_nilobject) {
			if (ttnov(val) == LUA_TROTABLE) {
				lua_pushrotable( L, val->value_.p);
			} else {
				lua_pushinteger(L, val->value_.i);
			}
			return 1;
		}
	}

	return (int)luaO_nilobject;
}

#endif

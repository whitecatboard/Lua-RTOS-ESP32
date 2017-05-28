/* Read-only tables for Lua */
#define LUAC_CROSS_FILE

#include "luartos.h"

#if LUA_USE_ROTABLE

#include "esp_attr.h"

#include "lapi.h"
#include "lauxlib.h"
#include "lobject.h"
#include "lrotable.h"
#include "cache.h"
#include "lstring.h"
#include "lua.h"
#include <string.h>

/* Externally defined read-only table array */
extern const luaR_entry lua_rotable[];

static const TValue *luaR_auxfind(const luaR_entry *pentry, const char *strkey,
		luaR_numkey numkey, unsigned *ppos);

/*
 * Only for debug purposes.
 */
void luaR_dump(luaR_entry *entry) {
	printf("lua_rotable %x\r\n",(unsigned int)entry);

	while (entry->key.id.strkey) {
		if (entry->key.len >= 0) {
			printf("  [%s] --> %x\r\n", entry->key.id.strkey,
					(unsigned int) rvalue(&entry->value));
		} else {
			printf("  [%d] --> %x\r\n", entry->key.id.numkey,
					(unsigned int) rvalue(&entry->value));
		}
		entry++;
	}
}

LUA_API void lua_pushrotable(lua_State *L, void *p) {
	lua_lock(L);
	setrvalue(L->top, p);
	api_incr_top(L); lua_unlock(L);
}

const TValue *luaL_rometatable(const void *data) {
	const TValue *res = luaR_auxfind((const luaR_entry *) data, "__metatable",
			0, NULL);

	return res && ttisrotable(res) ? rvalue(res) : NULL;
}

void luaA_pushobject(lua_State *L, const TValue *o) {
	setobj2s(L, L->top, o)
	;
	api_incr_top(L);
}

/* Find an entry in a rotable and return it */
static const IRAM_ATTR TValue *luaR_auxfind(const luaR_entry *pentry, const char *k, luaR_numkey nk, unsigned *ppos) {
	const TValue *res = NULL;
	const luaR_entry *entry = pentry;
	int i = 0;

	if (k) {
		// Try to get from cache
		#if CONFIG_LUA_RTOS_LUA_USE_ROTABLE_CACHE
		res = rotable_cache_get(pentry, k);
		if (res) {
			return res;
		}
		#endif

		int kl = strlen(k);

		while (entry->key.id.strkey) {
			if ((entry->key.type == LUA_TSTRING) && (entry->key.len == kl) && (!strncmp(entry->key.id.strkey, k, kl))) {
				#if CONFIG_LUA_RTOS_LUA_USE_ROTABLE_CACHE
				// Put in cache
				rotable_cache_put(pentry, entry);
				#endif

				res = &entry->value;
				break;
			}
			entry++;
			i++;
		}
	} else {
		while (entry->key.id.strkey) {
			if (i == nk) {
				res = &entry->value;
				break;
			}
			entry++;
			i++;
		}
	}

	if (res && ppos)
		*ppos = i;

	return res;
}

/**
 * @brief  Find a "read only table" in the lua_rotable array
 *
 * @param  name read only table name
 *
 * @return
 *     - If exists, the read only table value
 *     - If not exists, NULL
 *
 */
const IRAM_ATTR TValue *luaR_findglobal(const char *name) {
	// Try to get from cache
	#if CONFIG_LUA_RTOS_LUA_USE_ROTABLE_CACHE
	const TValue *res = NULL;

	res = rotable_cache_get(lua_rotable, name);
	if (res) {
		return res;
	}
	#endif

	const luaR_entry *entry = lua_rotable;
	int len = strlen(name);

	while (entry->key.id.strkey) {
		if ((entry->key.len == len) && (!strncmp(entry->key.id.strkey, name, len))) {
			#if CONFIG_LUA_RTOS_LUA_USE_ROTABLE_CACHE
			// Put in cache
			rotable_cache_put(lua_rotable, entry);
			#endif

			return &entry->value;
		}
		entry++;
	}

	return NULL;
}

int IRAM_ATTR luaR_findfunction(lua_State *L, const luaR_entry *ptable) {
	const TValue *res = NULL;
	const char *key = luaL_checkstring(L, 2);

	res = luaR_auxfind(ptable, key, 0, NULL);
	if (res && ttislcf(res)) {
		luaA_pushobject(L, res);
		return 1;
	} else
		return 0;
}

/* Find an entry in a rotable and return its type
 If "strkey" is not NULL, the function will look for a string key,
 otherwise it will look for a number key */
const TValue *luaR_findentry(const void *pentry, const char *strkey,
		luaR_numkey numkey, unsigned *ppos) {
	if (pentry) {
		return luaR_auxfind((const luaR_entry *) pentry, strkey, numkey, ppos);
	} else {
		return luaR_auxfind(lua_rotable, strkey, numkey, ppos);
	}
}
extern uint32_t _rodata_start;
extern uint32_t _lit4_end;
extern uint32_t _lua_rtos_rodata_start;
extern uint32_t _lua_rtos_rodata_end;

int luaR_isrotable(const void *p) {
	return (((&_rodata_start) <= (uint32_t *) p
			&& (uint32_t *) p <= (&_lit4_end))
			|| ((&_lua_rtos_rodata_start) <= (uint32_t *) p
					&& (uint32_t *) p <= (&_lua_rtos_rodata_end)));
}

int luaH_getn_ro(void *t) {
    luaR_entry *entry = (luaR_entry *)t;
    int len = 0;

    while (entry->key.id.strkey) {
		entry++;
		len++;
	}

	return len;
}

static void luaR_next_helper(lua_State *L, const luaR_entry *pentries, int pos,
		TValue *key, TValue *val) {
	setnilvalue(key);
	setnilvalue(val);
	if (pentries[pos].key.type != LUA_TNIL) {
		/* Found an entry */
		if (pentries[pos].key.type == LUA_TSTRING)
			setsvalue(L, key, luaS_newro( L, pentries[pos].key.id.strkey))
		else
			setnvalue(key, (lua_Number )pentries[pos].key.id.numkey)
		setobj2s(L, val, &pentries[pos].value)
		;
	}
}

/* next (used for iteration) */
void luaR_next(lua_State *L, void *data, TValue *key, TValue *val) {
	const luaR_entry *pentries = (const luaR_entry *) data;
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
			numkey = (luaR_numkey) nvalue(key);
		luaR_findentry(data, pstrkey, numkey, &keypos);
		/* Advance to next key */
		keypos++;
		luaR_next_helper(L, pentries, keypos, key, val);
	}
}

/* Convert a Lua string to a C string */
void luaR_getcstr(char *dest, const TString *src, size_t maxsize) {
	// TO DO: ??
	// There are 2 fields shrlen / lnglen
	// src->tsv.len+1 > maxsize

	if (src->shrlen + 1 > maxsize)
		dest[0] = '\0';
	else {
		memcpy(dest, getstr(src), src->shrlen);
		dest[src->shrlen] = '\0';
	}
}

int luaH_next_ro(lua_State *L, void *t, StkId key) {
	luaR_next(L, t, key, key + 1);
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
				lua_pushrotable(L, val->value_.p);
			} else {
				lua_pushinteger(L, val->value_.i);
			}
			return 1;
		}
	}

	return (int) luaO_nilobject;
}

LUALIB_API int luaL_newmetarotable (lua_State *L, const char* tname, void *p) {
  lua_getfield(L, LUA_REGISTRYINDEX, tname);  /* get registry.name */
  if (!lua_isnil(L, -1))  /* name already in use? */
    return 0;  /* leave previous value on top, but return 0 */
  lua_pop(L, 1);
  lua_pushrotable(L, p);
  lua_pushvalue(L, -1);
  lua_setfield(L, LUA_REGISTRYINDEX, tname);  /* registry.name = metatable */
  return 1;
}

#endif

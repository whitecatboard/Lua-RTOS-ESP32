/* Read-only tables helper */

#ifndef lrodefs_h
#define lrodefs_h

#include "lrotable.h"

#if LUA_USE_ROTABLE
#define LUA_REG_TYPE                luaR_entry
#define LSTRKEY                     LRO_STRKEY
#define LNUMKEY                     LRO_NUMKEY
#define LNILKEY                     LRO_NILKEY
#define LFUNCVAL                    LRO_FUNCVAL
#define LUDATA                      LRO_LUDATA
#define LNUMVAL                     LRO_NUMVAL
#define LINTVAL                     LRO_INTVAL
#define LROVAL                      LRO_ROVAL
#define LNILVAL                     LRO_NILVAL
#define LREGISTER(L, name, table)\
  return 0

#define LNEWLIB(L, name)\
  return 0

#else
#include "lauxlib.h"

#define LUA_REG_TYPE                luaL_Reg
#define LSTRKEY(x)                  (x)
#define LNILKEY                     NULL
#define LFUNCVAL(x)                 (x)
#define LNILVAL                     NULL
#define LNUMVAL(x)					(x)
#define LINTVAL(x)					(x)
#define LROVAL(x)					(x)
	  
#define LREGISTER(L, name, table)\
  luaL_register(L, name, table);\
  return 1

#define LNEWLIB(L, name)\
  luaL_newlib(L, #name);\
  return 1


#endif
	  
#endif /* lrodefs_h */

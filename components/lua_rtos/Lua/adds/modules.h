#ifndef _MODULES_H
#define _MODULES_H

#include "lrodefs.h"

#define PUT_IN_SECTION(s) __attribute__((used,unused,section(s))) 

// Macros for register a library
#define LIB_PASTER(x,y) x##y
#define LIB_EVALUATOR(x,y) LIB_PASTER(x,y)
#define LIB_CONCAT(x,y) LIB_EVALUATOR(x, y)

#define LIB_TOSTRING_PASTER(x) #x
#define LIB_TOSTRING_EVALUATOR(x) LIB_TOSTRING_PASTER(x)
#define LIB_TOSTRING(x) LIB_TOSTRING_EVALUATOR(x)

//#define LIB_USED(fname) LIB_CONCAT(CONFIG_LUA_RTOS_LUA_USE_,fname)
#define LIB_USED(fname) 1
#define LIB_INIT_DUMMY_FOO(fname)  LIB_CONCAT(fname,_init_dummy_foo)()
#define LIB_SECTION(fname, section) LIB_CONCAT(section,1) //LIB_USED(fname))

#if LUA_USE_ROTABLE

#define MODULE_REGISTER_UNMAPPED(fname, lname, func) \
extern void LIB_INIT_DUMMY_FOO(fname){ \
volatile static const PUT_IN_SECTION(LIB_TOSTRING(LIB_SECTION(fname,.lua_libs))) luaL_Reg LIB_CONCAT(lua_libs,LIB_CONCAT(_,LIB_CONCAT(lname,LIB_USED(fname)))) = {LIB_TOSTRING(lname), func}; \
} 

#define MODULE_REGISTER_MAPPED(fname, lname, map, func) \
extern void LIB_INIT_DUMMY_FOO(fname){ \
volatile static const PUT_IN_SECTION(LIB_TOSTRING(LIB_SECTION(fname,.lua_libs))) luaL_Reg LIB_CONCAT(lua_libs,LIB_CONCAT(_,LIB_CONCAT(lname,LIB_USED(fname)))) = {LIB_TOSTRING(lname), func}; \
volatile static const PUT_IN_SECTION(LIB_TOSTRING(LIB_SECTION(fname,.lua_rotable))) luaR_entry LIB_CONCAT(lua_rotable,LIB_CONCAT(_,LIB_CONCAT(lname,LIB_USED(fname)))) = {LSTRKEY(LIB_TOSTRING(lname)), LROVAL(map)}; \
} 

#define USE_LIB(fname) LIB_INIT_DUMMY_FOO(fname);

#else
#define MODULE_REGISTER(fname, lname, map)
#endif

#endif

#ifndef _MODULES_H
#define _MODULES_H

#include "lrodefs.h"

#define PUT_IN_SECTION(s) __attribute__((used,unused,section(s)))

// Macros for concatenate 2 tokens
#define LIB_PASTER(x,y) x##y
#define LIB_EVALUATOR(x,y) LIB_PASTER(x,y)
#define LIB_CONCAT(x,y) LIB_EVALUATOR(x,y)

// Macros for stringify a token
#define LIB_TOSTRING_PASTER(x) #x
#define LIB_TOSTRING_EVALUATOR(x) LIB_TOSTRING_PASTER(x)
#define LIB_TOSTRING(x) LIB_TOSTRING_EVALUATOR(x)

// Macros for register a library
#define LIB_USED(fname) LIB_CONCAT(LUA_USE_,fname)
#define LIB_SECTION(fname, section) LIB_CONCAT(section,LIB_USED(fname))

#define LIB_INIT(fname, lname, func) \
const PUT_IN_SECTION(LIB_TOSTRING(LIB_SECTION(fname,.lua_libs))) luaL_Reg LIB_CONCAT(lua_libs,LIB_CONCAT(_,LIB_CONCAT(lname,LIB_USED(fname)))) = {LIB_TOSTRING(lname), func}

#if LUA_USE_ROTABLE
#define LUA_OS_MODULE(fname, lname, map) \
const PUT_IN_SECTION(LIB_TOSTRING(LIB_SECTION(fname,.lua_libs))) luaL_Reg LIB_CONCAT(lua_libs,LIB_CONCAT(_,LIB_CONCAT(lname,LIB_USED(fname)))) = {LIB_TOSTRING(lname), LIB_CONCAT(luaopen_,lname)}; \
const PUT_IN_SECTION(LIB_TOSTRING(LIB_SECTION(fname,.lua_rotable))) luaR_entry LIB_CONCAT(lua_rotable,LIB_CONCAT(_,LIB_CONCAT(lname,LIB_USED(fname)))) = {LSTRKEY(LIB_TOSTRING(lname)), LROVAL(map)}
#else
#define LUA_OS_MODULE(fname, lname, map)
#endif

#endif

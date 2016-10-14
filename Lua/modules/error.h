#ifndef _LUA_ERROR_H
#define	_LUA_ERROR_H

#include "lstate.h"

#include <sys/drivers/error.h>
#include <sys/drivers/resource.h>

int luaL_driver_error(lua_State* L, const char *msg, tdriver_error *error);

#endif

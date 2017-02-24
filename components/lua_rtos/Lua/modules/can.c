/*
 * Lua RTOS, Lua CAN module
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, ƒintedata or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#if LUA_USE_CAN

#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"

#include <drivers/can/can.h>

static int lcan_pins( lua_State* L ) {
    return platform_can_pins(L);
}

// Lua: result = setup( id, clock )
static int lcan_setup( lua_State* L )
{
  unsigned id;
  u32 clock, res;
  
  id = luaL_checkinteger( L, 1 );
  MOD_CHECK_ID( can, id );
  
  clock = luaL_checkinteger( L, 2 );
  res = platform_can_setup( id, clock );
  if (res > 0) {
      lua_pushinteger( L, res );      
  } else {
      return luaL_error( L, "can't setup CAN" );
  }
  return 1;
}

// Lua: success = send( id, canid, canidtype, message )
static int lcan_send( lua_State* L ) {
  size_t len;
  int id, canid, idtype;
  const char *data;
  
  id = luaL_checkinteger( L, 1 );
  MOD_CHECK_ID( can, id );
  canid = luaL_checkinteger( L, 2 );
  idtype = luaL_checkinteger( L, 3 );
  len = luaL_checkinteger( L, 4 );
  
  data = luaL_checkstring (L, 5);
  if ( len > CAN_MAX_LEN )
    return luaL_error( L, "message exceeds max length" );
  
  platform_can_send( id, canid, idtype, len, ( const u8 * )data);
  
  return 0;
}

// Lua: canid, canidtype, message = recv( id )
static int lcan_recv( lua_State* L ) {
  u8 len;
  int id;
  u32 canid;
  u8  idtype, data[ CAN_MAX_LEN ];
  
  id = luaL_checkinteger( L, 1 );
  MOD_CHECK_ID( can, id );
  
  if( platform_can_recv( id, &canid, &idtype, &len, data ))
  {
    lua_pushinteger( L, canid );
    lua_pushinteger( L, idtype );
    lua_pushinteger( L, len );
    lua_pushlstring( L, ( const char * )data, ( size_t )len );
  
    return 4;
  }
  else
    return 0;
}

static int lcan_stats( lua_State* L )
{
  u8 len;
  int id;
  struct can_stats stats;
  
  id = luaL_checkinteger( L, 1 );
  MOD_CHECK_ID( can, id );
  
  if( platform_can_stats( id, &stats ))
  {
    lua_pushinteger( L, stats.rx );
    lua_pushinteger( L, stats.tx );
    lua_pushinteger( L, stats.rxqueued );
    lua_pushinteger( L, stats.rxunqueued );

    return 4;
  }
  else
    return 0;
}

// Module function map
#define MIN_OPT_LEVEL 2

const luaL_Reg can_map[] = 
{
  { "setup", lcan_setup },
  { "pins",  lcan_pins },
  { "send",  lcan_send },  
  { "recv",  lcan_recv },
  { "stats", lcan_stats },
  { NULL, NULL }
};

LUALIB_API int luaopen_can( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  luaL_register( L, AUXLIB_CAN, can_map );
  
  // Module constants  
  MOD_REG_INTEGER( L, "STD", 0 );
  MOD_REG_INTEGER( L, "EXT", 1 );
  
  int i;
  char buff[5];
 
  
  for(i=1;i<=NCAN;i++) {
      sprintf(buff,"CAN%d",i);
      MOD_REG_INTEGER( L, buff, i );
  }

  return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0  
}

#endif

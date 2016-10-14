// Module for interfacing with Lua SPI code
#include "whitecat.h"

#if LUA_USE_SPI

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"

#include "Lua/modules/spi.h"
#include "drivers/spi/spi.h"
#include "drivers/cpu/cpu.h"

// Lua: sson( id )
static int lspi_select( lua_State* L ) {
    spi_userdata *spi = NULL;

    spi = (spi_userdata *)luaL_checkudata(L, 1, "spi");
    luaL_argcheck(L, spi, 1, "spi expected");

    MOD_CHECK_ID( spi, spi->spi );
    platform_spi_select( spi, 1 );
    return 0;
}

// Lua: ssoff( id )
static int lspi_deselect( lua_State* L ) {
    spi_userdata *spi = NULL;

    spi = (spi_userdata *)luaL_checkudata(L, 1, "spi");
    luaL_argcheck(L, spi, 1, "spi expected");

    MOD_CHECK_ID( spi, spi->spi );
    platform_spi_select( spi, 0 );
    return 0;
}

// Lua: clock = setup( id, MASTER/SLAVE, clock, cpol, cpha, databits )
static int lspi_setup( lua_State* L )
{
  unsigned id, cpol, cpha, is_master, databits, cs;
  u32 clock, res;
  
  id = luaL_checkinteger( L, 1 );
  MOD_CHECK_ID( spi, id );
  is_master = luaL_checkinteger( L, 2 ) == 1;
  if( !is_master )
    return luaL_error( L, "invalid type (only spi.MASTER is supported)" );
  cs = luaL_checkinteger( L, 3 );
  clock = luaL_checkinteger( L, 4 );
  cpol = luaL_checkinteger( L, 5 );
  if( ( cpol != 0 ) && ( cpol != 1 ) )
    return luaL_error( L, "invalid clock polarity." );
  cpha = luaL_checkinteger( L, 6 );
  if( ( cpha != 0 ) && ( cpha != 1 ) )
    return luaL_error( L, "invalid clock phase." );
  databits = luaL_checkinteger( L, 7 );
  
  spi_userdata *spi = (spi_userdata *)lua_newuserdata(L, sizeof(spi_userdata));

  spi->spi = id;
  spi->cs = cs;
  spi->speed = clock;
  
  res = platform_spi_setup( spi, is_master, clock, cpol, cpha, databits );
  
  luaL_getmetatable(L, "spi");
  lua_setmetatable(L, -2);
  
  return 1;
}

static int lspi_pins( lua_State* L ) {
    return platform_spi_pins(L);
}

// Helper function: generic write/readwrite
static int lspi_rw_helper( lua_State *L, int withread ) {
  spi_data_type value;
  const char *sval; 
  spi_userdata *spi = NULL;
  int total = lua_gettop( L ), i, j, id;

  spi = (spi_userdata *)luaL_checkudata(L, 1, "spi");
  luaL_argcheck(L, spi, 1, "spi expected");
  
  size_t len, residx = 0;
  
  id = spi->spi;
  MOD_CHECK_ID( spi, id );
  if( withread )
    lua_newtable( L );
  for( i = 2; i <= total; i ++ )
  {
    if( lua_isnumber( L, i ) )
    {
      value = platform_spi_send_recv( spi, lua_tointeger( L, i ) );
      if( withread )
      {
        lua_pushinteger( L, value );
        lua_rawseti( L, -2, residx ++ );
      }
    }
    else if( lua_isstring( L, i ) )
    {
      sval = lua_tolstring( L, i, &len );
      for( j = 0; j < len; j ++ )
      {
        value = platform_spi_send_recv( spi, sval[ j ] );
        if( withread )
        {
          lua_pushinteger( L, value );
          lua_rawseti( L, -2, residx ++ );
        }
      }
    }
  }
  return withread ? 1 : 0;
}

// Lua: write( id, out1, out2, ... )
static int lspi_write( lua_State* L ) {
  return lspi_rw_helper( L, 0 );
}

// Lua: restable = readwrite( id, out1, out2, ... )
static int lspi_readwrite( lua_State* L ) {
  return lspi_rw_helper( L, 1 );
}

// Module function map
const luaL_Reg spi_method_map[] = 
{
  { "setup",  lspi_setup },
  { "pins",  lspi_pins },
  { "select",  lspi_select },
  { "deselect",  lspi_deselect },
  { "write",  lspi_write },  
  { "readwrite",  lspi_readwrite },    
  { NULL, NULL }
};

const luaL_Reg spi_map[] = {
  { NULL, NULL }
};

    
LUALIB_API int luaopen_spi( lua_State *L ) {
    int n;
    luaL_register( L, AUXLIB_SPI, spi_map );
    
    // Set it as its own metatable
    lua_pushvalue( L, -1 );
    lua_setmetatable( L, -2 );

    // create metatable
    luaL_newmetatable(L, "spi");

    // Module constants  
    int i;
    char buff[5];

    for(i=1;i<=NSPI;i++) {
        sprintf(buff,"SPI%d",i);
        MOD_REG_INTEGER( L, buff, i );
    }

    // Add the MASTER and SLAVE constants (for spi.setup)
    MOD_REG_INTEGER( L, "MASTER", 1 );
    MOD_REG_INTEGER( L, "SLAVE", 0 );  
  
    // metatable.__index = metatable
    lua_pushliteral(L, "__index");
    lua_pushvalue(L,-2);
    lua_rawset(L,-3);
    
    // Setup the methods inside metatable
    luaL_register( L, NULL, spi_method_map );

    return 1;      
}

#endif
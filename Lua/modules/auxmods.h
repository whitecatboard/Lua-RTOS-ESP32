// Auxiliary Lua modules. All of them are declared here, then each platform
// decides what module(s) to register in the src/platform/xxxxx/platform_conf.h file
// FIXME: no longer platform_conf.h - either CPU header file, or board file

#ifndef __AUXMODS_H__
#define __AUXMODS_H__

#include "lua.h"

#define AUXLIB_PIO      "pio"
LUALIB_API int ( luaopen_pio )( lua_State *L );

#define AUXLIB_SPI      "spi"
LUALIB_API int ( luaopen_spi )( lua_State *L );

#define AUXLIB_CAN      "can"
LUALIB_API int ( luaopen_can )( lua_State *L );

#define AUXLIB_TMR      "tmr"
LUALIB_API int ( luaopen_tmr )( lua_State *L );

#define AUXLIB_PD       "pd"
LUALIB_API int ( luaopen_pd )( lua_State *L );

#define AUXLIB_UART     "uart"
LUALIB_API int ( luaopen_uart )( lua_State *L );

#define AUXLIB_PWM      "pwm"
LUALIB_API int ( luaopen_pwm )( lua_State *L );

#define AUXLIB_NET      "net"
LUALIB_API int ( luaopen_net )( lua_State *L );

#define AUXLIB_CPU      "cpu"
LUALIB_API int ( luaopen_cpu )( lua_State* L );

#define AUXLIB_ADC      "adc"
LUALIB_API int ( luaopen_adc )( lua_State *L );

#define AUXLIB_MQTT  "mqtt"
LUALIB_API int ( luaopen_mqtt )( lua_State *L );

#define AUXLIB_THREAD "thread"
LUALIB_API int (luaopen_thread) (lua_State* L);

#define AUXLIB_SCREEN "screen"
LUALIB_API int (luaopen_screen) (lua_State* L);

#define AUXLIB_GPS "gps"
LUALIB_API int (luaopen_gps) (lua_State* L);

#define AUXLIB_HTTP "http"
LUALIB_API int (luaopen_http) (lua_State* L);

#define AUXLIB_STEPPER "stepper"
LUALIB_API int (luaopen_stepper) (lua_State* L);

#define AUXLIB_I2C "i2c"
LUALIB_API int (luaopen_i2c) (lua_State* L);

#define AUXLIB_LORA "lora"
LUALIB_API int (luaopen_lora) (lua_State* L);

#define AUXLIB_PACK "pack"
LUALIB_API int (luaopen_pack) (lua_State* L);

#define AUXLIB_NVS "nvs"
LUALIB_API int (luaopen_nvs) (lua_State* L);

// Helper macros
#define MOD_CHECK_ID( mod, id )\
  if( !platform_ ## mod ## _exists( id ) )\
    return luaL_error( L, #mod" %d does not exist", ( unsigned )id )

#define MOD_REG_INTEGER( L, name, val )\
  lua_pushinteger( L, val );\
  lua_setfield( L, -2, name )

#endif


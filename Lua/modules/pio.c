// Module for interfacing with PIO

#if LUA_USE_PIO

#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "pio.h"
#include "modules.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "sys/drivers/cpu.h"
#include "sys/drivers/gpio.h"

// PIO public constants
#define PIO_DIR_OUTPUT      0
#define PIO_DIR_INPUT       1

// PIO private constants
#define PIO_PORT_OP         0
#define PIO_PIN_OP          1

// Helper functions
static pio_type pio_op(unsigned port, pio_type pinmask, int op) {
	switch (op) {
		case PLATFORM_IO_PIN_DIR_INPUT:
			gpio_pin_input_mask(port, pinmask);
			return 1;
			break;

		case PLATFORM_IO_PORT_DIR_INPUT:
			gpio_port_input(port);
			return 1;
			break;
			
		case PLATFORM_IO_PIN_DIR_OUTPUT:
			gpio_pin_output_mask(port, pinmask);
			return 1;
			break;

		case PLATFORM_IO_PORT_DIR_OUTPUT:
			gpio_port_output(port);
			return 1;
			break;
			
		case PLATFORM_IO_PIN_SET:
			gpio_pin_set_mask(port, pinmask);
			return 1;
			break;
			
		case PLATFORM_IO_PORT_SET_VALUE:
			gpio_port_set(port, pinmask);
			return 1;
			break;
			
		case PLATFORM_IO_PIN_CLEAR:
			gpio_pin_clr_mask(port, pinmask);
			return 1;
			break;
			
		case PLATFORM_IO_PIN_GET:
			return gpio_pin_get_mask(port, pinmask);
			break;

		case PLATFORM_IO_PORT_GET_VALUE:
			return gpio_port_get(port);
			break;

        case PLATFORM_IO_PIN_PULLUP:
			gpio_pin_pullup_mask(port, pinmask);
            return 1;
            
        case PLATFORM_IO_PIN_PULLDOWN:
			gpio_pin_pulldwn_mask(port, pinmask);
            return 1;
            
        case PLATFORM_IO_PIN_NOPULL:
			gpio_pin_nopull_mask(port, pinmask);
            return 1;
	}
	
	return 0;
}

static int pioh_set_pins(lua_State* L, int stackidx, int op) {
  int total = lua_gettop(L);
  int i, v, port, pin;

  pio_type pio_masks[GPIO_PORTS];
 
  for(i = 0; i < GPIO_PORTS; i ++)
    pio_masks[i] = 0;
  
  // Get all masks
  for(i = stackidx; i <= total; i ++) {
    v = luaL_checkinteger(L, i);
    
    // Get port and pin inside this port
    port = cpu_port_number(v);
    pin = cpu_pin_number(v);
    
    // Test if this port / pin exists
    if (!cpu_has_gpio(port, pin)) {
      return luaL_error(L, "invalid pin");
    }
    
    pio_masks[port] |= 1 << pin;
  }
  
  // Execute the given operation
  for(i = 0; i < GPIO_PORTS; i ++)
    if(pio_masks[i])
      if(!pio_op(i, pio_masks[i], op))
        return luaL_error(L, "invalid PIO operation");

  return 0;
}

static int pioh_get_pins(lua_State* L, int stackidx, int op) {
  int total = lua_gettop(L);
  int i, v, port, pin;
  unsigned int val;

  pio_type pio_masks[GPIO_PORTS];
 
  for(i = 0; i < GPIO_PORTS; i ++)
    pio_masks[i] = 0;
  
  // Get all masks
  for(i = stackidx; i <= total; i ++) {
    v = luaL_checkinteger(L, i);
    
    // Get port and pin inside this port
    port = cpu_port_number(v);
    pin = cpu_pin_number(v);
    
    // Test if this port / pin exists
    if (!cpu_has_gpio(port, pin)) {
      return luaL_error(L, "invalid pin");
    }
    
    pio_masks[port] |= 1 << pin;
  }
  
  // Execute the given operation
  for(i = 0; i < GPIO_PORTS; i ++) {
     if(pio_masks[i]) {
      	unsigned int mask = 1;

    	val = pio_op(i, pio_masks[i], op);

      	for(i=0; i < GPIO_PER_PORT; i++) {
      		if (pio_masks[i] & mask) {
      			if (val & mask) {
      				 lua_pushinteger(L, 1);
      			} else {
      				 lua_pushinteger(L, 0);
      			}
      		}
      		
      		mask = (mask << 1);
      	}
     }
  }

  return total;
}

static int pioh_set_ports(lua_State* L, int stackidx, int op, pio_type mask) {
  int total = lua_gettop(L);
  int i, v, port;
  u32_t port_mask = 0;

  // Get all masks
  for(i = stackidx; i <= total; i ++) {
    v = luaL_checkinteger(L, i);
    port = cpu_port_number(v);
    if(!cpu_has_port(port))
      return luaL_error(L, "invalid port");
    port_mask |= (1ULL << port);
  }
  
  // Ask platform to execute the given operation
  for(i = 0; i < GPIO_PORTS; i ++)
    if(port_mask & (1ULL << i))
      if(!pio_op(i, mask, op))
        return luaL_error(L, "invalid PIO operation");

  return 0;
}

static int pio_gen_setdir(lua_State *L, int optype) {
  int op = luaL_checkinteger(L, 1);

  if(op == PIO_DIR_INPUT)
    op = optype == PIO_PIN_OP ? PLATFORM_IO_PIN_DIR_INPUT : PLATFORM_IO_PORT_DIR_INPUT;
  else if(op == PIO_DIR_OUTPUT)
    op = optype == PIO_PIN_OP ? PLATFORM_IO_PIN_DIR_OUTPUT : PLATFORM_IO_PORT_DIR_OUTPUT;
  else
    return luaL_error(L, "invalid direction");
  if(optype == PIO_PIN_OP)
    return pioh_set_pins(L, 2, op);
  else
    return pioh_set_ports(L, 2, op, GPIO_ALL);
}

static int pio_gen_setpull(lua_State *L, int optype) {
	int op = luaL_checkinteger(L, 1);

  	if((op != PLATFORM_IO_PIN_PULLUP) &&
      (op != PLATFORM_IO_PIN_PULLDOWN) &&
      (op != PLATFORM_IO_PIN_NOPULL))
    	return luaL_error(L, "invalid pull type");
  	if(optype == PIO_PIN_OP)
    	return pioh_set_pins(L, 2, op);
  	else
    	return pioh_set_ports(L, 2, op, GPIO_ALL);
}

static int pio_gen_setval(lua_State *L, int optype, pio_type val, int stackidx) {
	if((optype == PIO_PIN_OP) && (val != 1) && (val != 0)) 
    	return luaL_error(L, "invalid pin value");
  	if(optype == PIO_PIN_OP)
    	return pioh_set_pins(L, stackidx, val == 1 ? PLATFORM_IO_PIN_SET : PLATFORM_IO_PIN_CLEAR);
  	else
    	return pioh_set_ports(L, stackidx, PLATFORM_IO_PORT_SET_VALUE, val);
}

// Module functions
static int pio_pin_setdir(lua_State *L) {
	return pio_gen_setdir(L, PIO_PIN_OP);
}

static int pio_pin_output(lua_State *L) {
	return pioh_set_pins(L, 1, PLATFORM_IO_PIN_DIR_OUTPUT);
}

static int pio_pin_input(lua_State *L) {
	return pioh_set_pins(L, 1, PLATFORM_IO_PIN_DIR_INPUT);
}

static int pio_pin_setpull(lua_State *L) {
	return pio_gen_setpull(L, PIO_PIN_OP);
}

static int pio_pin_setval(lua_State *L) {
  pio_type val = (pio_type)luaL_checkinteger(L, 1);

  return pio_gen_setval(L, PIO_PIN_OP, val, 2);
}

static int pio_pin_sethigh(lua_State *L) {
	return pio_gen_setval(L, PIO_PIN_OP, 1, 1);
}

static int pio_pin_setlow(lua_State *L) {
	return pio_gen_setval(L, PIO_PIN_OP, 0, 1);
}

static int pio_pin_getval(lua_State *L) {
	return pioh_get_pins(L, 1, PLATFORM_IO_PIN_GET);
}

static int pio_pin_pinnum(lua_State *L) {
  pio_type value;
 
  int v, i, port, pin;
  int total = lua_gettop(L);
  
  for(i = 1; i <= total; i ++) {
    v = luaL_checkinteger(L, i);  

    port = cpu_port_number(v);
    pin = cpu_pin_number(v);
    
    // Test if this port / pin exists
    if (!cpu_has_gpio(port, pin)) {
      return luaL_error(L, "invalid pin");
    } else {
      value = cpu_pin_number(v);
      lua_pushinteger(L, value);
    }
  }
  return total;
}

static int pio_port_setdir(lua_State *L) {
	return pio_gen_setdir(L, PIO_PORT_OP);
}

static int pio_port_output(lua_State *L) {
	return pioh_set_ports(L, 1, PLATFORM_IO_PIN_DIR_OUTPUT, GPIO_ALL);
}

static int pio_port_input(lua_State *L) {
	return pioh_set_ports(L, 1, PLATFORM_IO_PIN_DIR_INPUT, GPIO_ALL);
}

static int pio_port_setpull(lua_State *L) {
	return pio_gen_setpull(L, PIO_PORT_OP);
}

static int pio_port_setval(lua_State *L) {
	pio_type val = (pio_type)luaL_checkinteger(L, 1);

	return pio_gen_setval(L, PIO_PORT_OP, val, 2);
}

static int pio_port_sethigh(lua_State *L) {
	return pio_gen_setval(L, PIO_PORT_OP, GPIO_ALL, 1);
}

static int pio_port_setlow(lua_State *L) {
	return pio_gen_setval(L, PIO_PORT_OP, 0, 1);
}

static int pio_port_getval(lua_State *L) {
  pio_type value;
  int v, i, port;
  int total = lua_gettop(L);
  
  for(i = 1; i <= total; i ++) {
    v = luaL_checkinteger(L, i);  
    port = cpu_port_number(v);
    if(!cpu_has_port(port))
      return luaL_error(L, "invalid port");
    else {
      value = pio_op(port, GPIO_ALL, PLATFORM_IO_PORT_GET_VALUE);
      lua_pushinteger(L, value);
    }
  }
  return total;
}

static int pio_decode(lua_State *L) {
	int code = (int)luaL_checkinteger(L, 1);
  	int port = cpu_port_number(code);
  	int pin  = cpu_pin_number(code);

	lua_pushinteger(L, port);
  	lua_pushinteger(L, pin);
	
	return 2;
}

#include "modules.h"

static const LUA_REG_TYPE pio_pin_map[] = {
    { LSTRKEY( "setdir"  ),			LFUNCVAL( pio_pin_setdir  ) },
    { LSTRKEY( "output"  ),			LFUNCVAL( pio_pin_output  ) },
    { LSTRKEY( "input"   ),			LFUNCVAL( pio_pin_input   ) },
    { LSTRKEY( "setpull" ),			LFUNCVAL( pio_pin_setpull ) },
    { LSTRKEY( "setval"  ),			LFUNCVAL( pio_pin_setval  ) },
    { LSTRKEY( "sethigh" ),			LFUNCVAL( pio_pin_sethigh ) },
    { LSTRKEY( "setlow"  ),			LFUNCVAL( pio_pin_setlow  ) },
    { LSTRKEY( "getval"  ),			LFUNCVAL( pio_pin_getval  ) },
    { LSTRKEY( "num"  	 ),			LFUNCVAL( pio_pin_pinnum  ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE pio_port_map[] = {
    { LSTRKEY( "setdir"  ),			LFUNCVAL( pio_port_setdir  ) },
    { LSTRKEY( "output"  ),			LFUNCVAL( pio_port_output  ) },
    { LSTRKEY( "input"   ),			LFUNCVAL( pio_port_input   ) },
    { LSTRKEY( "setpull" ),			LFUNCVAL( pio_port_setpull ) },
    { LSTRKEY( "setval"  ),			LFUNCVAL( pio_port_setval  ) },
    { LSTRKEY( "sethigh" ),			LFUNCVAL( pio_port_sethigh ) },
    { LSTRKEY( "setlow"  ),			LFUNCVAL( pio_port_setlow  ) },
    { LSTRKEY( "getval"  ),			LFUNCVAL( pio_port_getval  ) },
    { LNILKEY, LNILVAL }
};

#if !LUA_USE_ROTABLE
static const LUA_REG_TYPE pio_map[] = {
    { LSTRKEY( "decode"   ),		LFUNCVAL( pio_decode       ) },
    { LNILKEY, LNILVAL }
};
#else
static const LUA_REG_TYPE pio_map[] = {
    { LSTRKEY( "decode"   ),		LFUNCVAL( pio_decode               ) },
	{ LSTRKEY( "pin"      ), 	   	LROVAL  ( pio_pin_map              ) },
	{ LSTRKEY( "port"     ), 	   	LROVAL  ( pio_port_map             ) },

    { LSTRKEY( "INPUT"    ),		LINTVAL ( PIO_DIR_INPUT            ) },
    { LSTRKEY( "OUTPUT"   ),		LINTVAL ( PIO_DIR_OUTPUT           ) },
    { LSTRKEY( "PULLUP"   ),		LINTVAL ( PLATFORM_IO_PIN_PULLUP   ) },
    { LSTRKEY( "PULLDOWN" ),		LINTVAL ( PLATFORM_IO_PIN_PULLDOWN ) },
    { LSTRKEY( "NOPULL"   ),		LINTVAL ( PLATFORM_IO_PIN_NOPULL   ) },

    PIO_GPIO1
    PIO_GPIO2
    PIO_GPIO3
    PIO_GPIO4
    PIO_GPIO5
    PIO_GPIO6
    PIO_GPIO7
    PIO_GPIO8
    PIO_GPIO9
    PIO_GPIO10
    PIO_GPIO11
    PIO_GPIO12
    PIO_GPIO13
    PIO_GPIO14
    PIO_GPIO15
    PIO_GPIO16

    { LNILKEY, LNILVAL }
};
#endif

LUALIB_API int luaopen_pio(lua_State *L) {
#if !LUA_USE_ROTABLE
    luaL_newlib(L, pio_map);

    // Set it as its own metatable
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
  
    // Set constants for direction/pullups
    lua_pushinteger(L, PIO_DIR_INPUT);
    lua_setfield(L, -2, "INPUT");

    lua_pushinteger(L, PIO_DIR_OUTPUT);
    lua_setfield(L, -2, "OUTPUT");

    lua_pushinteger(L, PLATFORM_IO_PIN_PULLUP);
    lua_setfield(L, -2, "PULLUP");

    lua_pushinteger(L, PLATFORM_IO_PIN_PULLDOWN);
    lua_setfield(L, -2, "PULLDOWN");

    lua_pushinteger(L, PLATFORM_IO_PIN_NOPULL);
    lua_setfield(L, -2, "NOPULL");

    int port, pin;
    char tmp[6];
  
    // Set constants for port names
    for(port=1;port <= 8; port++) {
        if (platform_pio_has_port(port)) {
            sprintf(tmp,"P%c", platform_pio_port_name(port));
            lua_pushinteger(L, (port << 4));
            lua_setfield(L, -2, tmp);
        }
    }

    // Set constants for pin names
    for(port=1;port <= 8; port++) {
        if (platform_pio_has_port(port)) {
            for(pin=0;pin < 16;pin++) {
                if (platform_pio_has_pin(port, pin)) {   
                    sprintf(tmp,"P%c_%d", platform_pio_port_name(port), pin);
                    lua_pushinteger(L, (port << 4) | pin);
                    lua_setfield(L, -2, tmp);
                }
            }
        }
    }
    
    // Setup the new tables (pin and port) inside pio
    lua_newtable(L);
    luaL_register(L, NULL, pio_pin_map);
    lua_setfield(L, -2, "pin");

    lua_newtable(L);
    luaL_register(L, NULL, pio_port_map);
    lua_setfield(L, -2, "port");

    return 1;
#else
	return 0;
#endif
}

LUA_OS_MODULE(PIO, pio, pio_map);

#endif
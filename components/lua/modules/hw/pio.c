// Module for interfacing with PIO

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_PIO

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "error.h"
#include "lualib.h"
#include "lauxlib.h"
#include "pio.h"
#include "sys.h"
#include "modules.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <sys/status.h>

#include <drivers/cpu.h>
#include <drivers/gpio.h>

#include "esp_intr.h"

// PIO public constants
#define PIO_DIR_OUTPUT      0
#define PIO_DIR_INPUT       1

// PIO private constants
#define PIO_PORT_OP         0
#define PIO_PIN_OP          1

typedef struct {
    uint8_t pin;
    uint8_t type;
    lua_callback_t *callback;
    TaskHandle_t task;
} pio_intr_t;

typedef struct {
    uint8_t value;
} pio_intr_data_t;

static void IRAM_ATTR pio_intr_handler(void* arg) {
    portBASE_TYPE high_priority_task_awoken = 0;
    pio_intr_t *args = (pio_intr_t *)arg;
    uint32_t data;

    if (args->pin < 40) {
        gpio_intr_disable(args->pin);
    }

    // Get pin value
    switch (args->type) {
        case GPIO_INTR_POSEDGE: data = 1; break;
        case GPIO_INTR_NEGEDGE: data = 0; break;
        case GPIO_INTR_ANYEDGE: data = gpio_ll_pin_get(args->pin); break;
        case GPIO_INTR_LOW_LEVEL: data = 0; break;
        case GPIO_INTR_HIGH_LEVEL: data =1; break;
    }

    xTaskNotifyFromISR(args->task, data, eSetValueWithOverwrite, &high_priority_task_awoken);

    // Try to exec deferred callback as soon as possible
    if(high_priority_task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void pioTask(void *taskArgs) {
    pio_intr_t *args = (pio_intr_t *)taskArgs;
    uint32_t data;

    for(;;) {
        xTaskNotifyWait(0, 0, &data, portMAX_DELAY);

        lua_pushinteger(luaS_callback_state(args->callback), data);
        luaS_callback_call(args->callback, 1);

        if (args->pin < 40) {
            gpio_intr_enable(args->pin);
        }
    }

    luaS_callback_destroy(args->callback);
}

// Helper functions
//
// port goes from 1 to GPIO_PORTS
static int pio_op(lua_State *L, unsigned port, gpio_pin_mask_t pinmask, int op, gpio_pin_mask_t *value) {
    driver_error_t *error;

    switch (op) {
        case PLATFORM_IO_PIN_DIR_INPUT:
            if ((error = gpio_pin_input_mask(port, pinmask))) {
                return luaL_driver_error(L, error);
            }
            break;

        case PLATFORM_IO_PORT_DIR_INPUT:
            if ((error = gpio_port_input(port))) {
                return luaL_driver_error(L, error);
            }
            break;

        case PLATFORM_IO_PIN_DIR_OUTPUT:
            if ((error = gpio_pin_output_mask(port, pinmask))) {
                return luaL_driver_error(L, error);
            }
            break;

        case PLATFORM_IO_PORT_DIR_OUTPUT:
            if ((error = gpio_port_output(port))) {
                return luaL_driver_error(L, error);
            }
            break;

        case PLATFORM_IO_PIN_SET:
            if ((error = gpio_pin_set_mask(port, pinmask))) {
                return luaL_driver_error(L, error);
            }
            break;

        case PLATFORM_IO_PORT_SET_VALUE:
            if ((error = gpio_port_set(port, pinmask))) {
                return luaL_driver_error(L, error);
            }
            break;


        case PLATFORM_IO_PIN_INV:
            if ((error = gpio_pin_inv_mask(port, pinmask))) {
                return luaL_driver_error(L, error);
            }
            break;

        case PLATFORM_IO_PIN_CLEAR:
            if ((error = gpio_pin_clr_mask(port, pinmask))) {
                return luaL_driver_error(L, error);
            }
            break;

        case PLATFORM_IO_PIN_GET:
            if ((error = gpio_pin_get_mask(port, pinmask, value))) {
                return luaL_driver_error(L, error);
            }
            break;

        case PLATFORM_IO_PORT_GET_VALUE:
            if ((error = gpio_port_get(port, value))) {
                return luaL_driver_error(L, error);
            }
            break;

        case PLATFORM_IO_PIN_PULLUP:
            if ((error = gpio_pin_pullup_mask(port, pinmask))) {
                return luaL_driver_error(L, error);
            }
            break;

        case PLATFORM_IO_PIN_PULLDOWN:
            if ((error = gpio_pin_pulldwn_mask(port, pinmask))) {
                return luaL_driver_error(L, error);
            }
            break;

        case PLATFORM_IO_PIN_NOPULL:
            if ((error = gpio_pin_nopull_mask(port, pinmask))) {
                return luaL_driver_error(L, error);
            }
            break;
    }

    return 0;
}

static int pioh_set_pins(lua_State* L, int stackidx, int op) {
  int total = lua_gettop(L);
  int i, v, port, pin;

  gpio_pin_mask_t pio_masks[GPIO_PORTS];
 
  for(i = 0; i < GPIO_PORTS; i ++)
    pio_masks[i] = 0;
  
  // Get all masks
  for(i = stackidx; i <= total; i ++) {
    v = luaL_checkinteger(L, i);
    
    // Get port and pin inside this port
    port = cpu_port_number(v);
    pin = cpu_gpio_number(v);
    
    // Test if this port / pin exists
    if (!cpu_has_gpio(port, pin)) {
      return luaL_error(L, "invalid pin");
    }

    pio_masks[port - 1] |= GPIO_BIT_MASK << pin;
  }
  
  // Execute the given operation
  for(i = 0; i < GPIO_PORTS; i ++)
    if(pio_masks[i])
      pio_op(L, i + 1, pio_masks[i], op, NULL);

  return 0;
}

static int pioh_get_pins(lua_State* L, int stackidx, int op) {
  int total = lua_gettop(L);
  int i, j, v, port, pin;
  gpio_pin_mask_t val;

  gpio_pin_mask_t pio_masks[GPIO_PORTS];
 
  for(i = 0; i < GPIO_PORTS; i ++)
    pio_masks[i] = 0;
  
  // Get all masks
  for(i = stackidx; i <= total; i ++) {
    v = luaL_checkinteger(L, i);
    
    // Get port and pin inside this port
    port = cpu_port_number(v);
    pin = cpu_gpio_number(v);
    
    // Test if this port / pin exists
    if (!cpu_has_gpio(port, pin)) {
      return luaL_error(L, "invalid pin");
    }
    
    pio_masks[port - 1] |= GPIO_BIT_MASK << pin;
  }
  
  // Execute the given operation
  for(i = 0; i < GPIO_PORTS; i ++) {
     if(pio_masks[i]) {
         gpio_pin_mask_t mask = GPIO_BIT_MASK;

        pio_op(L, i + 1, pio_masks[i], op, &val);

          for(j=0; j < GPIO_PER_PORT; j++) {
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

static int pioh_set_ports(lua_State* L, int stackidx, int op, gpio_pin_mask_t mask) {
  int total = lua_gettop(L);
  int i, v, port;
  uint32_t port_mask = 0;

  // Get all masks
  for(i = stackidx; i <= total; i ++) {
    v = luaL_checkinteger(L, i);
    port = cpu_port_number(v);
    if(!cpu_has_port(port))
      return luaL_error(L, "invalid port");
    port_mask |= (1 << port);
  }
  
  // Ask platform to execute the given operation
  for(i = 0; i < GPIO_PORTS; i ++)
    if(port_mask & (1 << i))
      pio_op(L, i + 1, mask, op, NULL);

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

static int pio_gen_setval(lua_State *L, int optype, gpio_pin_mask_t val, int stackidx) {
    if((optype == PIO_PIN_OP) && (val != 1) && (val != 0))
        return luaL_error(L, "invalid pin value");
      if(optype == PIO_PIN_OP)
        return pioh_set_pins(L, stackidx, val == 1 ? PLATFORM_IO_PIN_SET : PLATFORM_IO_PIN_CLEAR);
      else
        return pioh_set_ports(L, stackidx, PLATFORM_IO_PORT_SET_VALUE, val);
}

static int pio_gen_invval(lua_State *L, int optype, gpio_pin_mask_t val, int stackidx) {
    return pioh_set_pins(L, stackidx, PLATFORM_IO_PIN_INV);
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
  gpio_pin_mask_t val = (gpio_pin_mask_t)luaL_checkinteger(L, 1);

  return pio_gen_setval(L, PIO_PIN_OP, val, 2);
}

static int pio_pin_sethigh(lua_State *L) {
    return pio_gen_setval(L, PIO_PIN_OP, 1, 1);
}

static int pio_pin_setlow(lua_State *L) {
    return pio_gen_setval(L, PIO_PIN_OP, 0, 1);
}

static int pio_pin_invval(lua_State *L) {
    return pio_gen_invval(L, PIO_PIN_OP, 0, 1);
}

static int pio_pin_getval(lua_State *L) {
    return pioh_get_pins(L, 1, PLATFORM_IO_PIN_GET);
}

static int pio_pin_pinnum(lua_State *L) {
  gpio_pin_mask_t value;
 
  int v, i, port, pin;
  int total = lua_gettop(L);
  
  for(i = 1; i <= total; i ++) {
    v = luaL_checkinteger(L, i);  

    port = cpu_port_number(v);
    pin = cpu_gpio_number(v);
    
    // Test if this port / pin exists
    if (!cpu_has_gpio(port, pin)) {
      return luaL_error(L, "invalid pin");
    } else {
      value = cpu_gpio_number(v);
      lua_pushinteger(L, value);
    }
  }
  return total;
}

static int pio_pin_interrupt(lua_State *L) {
    pio_intr_t *args;

    uint32_t pin = luaL_checkinteger(L, 1);
    int type = luaL_optinteger(L, 3, GPIO_INTR_POSEDGE);
    int stack = luaL_optinteger(L, 4, CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE);
    int priority = luaL_optinteger(L, 5, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY);

    args = (pio_intr_t *)calloc(1,sizeof(pio_intr_t));
    if (!args) {
        return luaL_exception(L, GPIO_ERR_NOT_ENOUGH_MEMORY);
    }

    args->callback = luaS_callback_create(L, 2);
    if (args->callback == NULL) {
        free(args);
        return luaL_exception(L, GPIO_ERR_NOT_ENOUGH_MEMORY);
    }

    args->pin = pin;
    args->type = type;

    // ISR related task must run on the same core that ISR handler is added
    xTaskCreatePinnedToCore(pioTask, "lpio", stack, args, priority, &args->task, xPortGetCoreID());

    gpio_isr_attach(pin, pio_intr_handler, type, args);

    return 0;
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
    gpio_pin_mask_t val = (gpio_pin_mask_t)luaL_checkinteger(L, 1);

    return pio_gen_setval(L, PIO_PORT_OP, val, 2);
}

static int pio_port_sethigh(lua_State *L) {
    return pio_gen_setval(L, PIO_PORT_OP, GPIO_ALL, 1);
}

static int pio_port_setlow(lua_State *L) {
    return pio_gen_setval(L, PIO_PORT_OP, 0, 1);
}

static int pio_port_getval(lua_State *L) {
  gpio_pin_mask_t value;
  int v, i, port;
  int total = lua_gettop(L);
  
  for(i = 1; i <= total; i ++) {
    v = luaL_checkinteger(L, i);  
    port = cpu_port_number(v);
    if(!cpu_has_port(port))
      return luaL_error(L, "invalid port");
    else {
      pio_op(L, port, GPIO_ALL, PLATFORM_IO_PORT_GET_VALUE, &value);
      lua_pushinteger(L, value);
    }
  }
  return total;
}

static int pio_decode(lua_State *L) {
    int code = (int)luaL_checkinteger(L, 1);
      int port = cpu_port_number(code);
      int pin  = cpu_gpio_number(code);

    lua_pushinteger(L, port);
      lua_pushinteger(L, pin);

    return 2;
}

#include "modules.h"

static const LUA_REG_TYPE pio_pin_map[] = {
    { LSTRKEY( "setdir"    ),            LFUNCVAL( pio_pin_setdir     ) },
    { LSTRKEY( "output"    ),            LFUNCVAL( pio_pin_output     ) },
    { LSTRKEY( "input"     ),            LFUNCVAL( pio_pin_input      ) },
    { LSTRKEY( "setpull"   ),            LFUNCVAL( pio_pin_setpull    ) },
    { LSTRKEY( "setval"    ),            LFUNCVAL( pio_pin_setval     ) },
    { LSTRKEY( "sethigh"   ),            LFUNCVAL( pio_pin_sethigh    ) },
    { LSTRKEY( "setlow"    ),            LFUNCVAL( pio_pin_setlow     ) },
    { LSTRKEY( "inv"       ),            LFUNCVAL( pio_pin_invval     ) },
    { LSTRKEY( "getval"    ),            LFUNCVAL( pio_pin_getval     ) },
    { LSTRKEY( "num"         ),          LFUNCVAL( pio_pin_pinnum     ) },
    { LSTRKEY( "interrupt" ),            LFUNCVAL( pio_pin_interrupt  ) },
    { LSTRKEY( "IntrPosEdge"   ),        LINTVAL ( GPIO_INTR_POSEDGE        ) },
    { LSTRKEY( "IntrNegEdge"   ),        LINTVAL ( GPIO_INTR_NEGEDGE        ) },
    { LSTRKEY( "IntrAnyEdge"   ),        LINTVAL ( GPIO_INTR_ANYEDGE        ) },
    { LSTRKEY( "IntrLowLevel"  ),        LINTVAL ( GPIO_INTR_LOW_LEVEL      ) },
    { LSTRKEY( "IntrHighLevel" ),        LINTVAL ( GPIO_INTR_HIGH_LEVEL     ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE pio_port_map[] = {
    { LSTRKEY( "setdir"  ),            LFUNCVAL( pio_port_setdir  ) },
    { LSTRKEY( "output"  ),            LFUNCVAL( pio_port_output  ) },
    { LSTRKEY( "input"   ),            LFUNCVAL( pio_port_input   ) },
    { LSTRKEY( "setpull" ),            LFUNCVAL( pio_port_setpull ) },
    { LSTRKEY( "setval"  ),            LFUNCVAL( pio_port_setval  ) },
    { LSTRKEY( "sethigh" ),            LFUNCVAL( pio_port_sethigh ) },
    { LSTRKEY( "setlow"  ),            LFUNCVAL( pio_port_setlow  ) },
    { LSTRKEY( "getval"  ),            LFUNCVAL( pio_port_getval  ) },
    { LNILKEY, LNILVAL }
};

#if !LUA_USE_ROTABLE
static const LUA_REG_TYPE pio_map[] = {
    { LSTRKEY( "decode"   ),        LFUNCVAL( pio_decode       ) },
    { LNILKEY, LNILVAL }
};
#else
static const LUA_REG_TYPE pio_map[] = {
    { LSTRKEY( "decode"   ),        LFUNCVAL( pio_decode               ) },
    { LSTRKEY( "pin"      ),        LROVAL  ( pio_pin_map              ) },
    { LSTRKEY( "port"     ),        LROVAL  ( pio_port_map             ) },

    { LSTRKEY( "INPUT"    ),        LINTVAL ( PIO_DIR_INPUT            ) },
    { LSTRKEY( "OUTPUT"   ),        LINTVAL ( PIO_DIR_OUTPUT           ) },
    { LSTRKEY( "PULLUP"   ),        LINTVAL ( PLATFORM_IO_PIN_PULLUP   ) },
    { LSTRKEY( "PULLDOWN" ),        LINTVAL ( PLATFORM_IO_PIN_PULLDOWN ) },
    { LSTRKEY( "NOPULL"   ),        LINTVAL ( PLATFORM_IO_PIN_NOPULL   ) },

    PIO_GPIO0
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
    PIO_GPIO17
    PIO_GPIO18
    PIO_GPIO19
    PIO_GPIO20
    PIO_GPIO21
    PIO_GPIO22
    PIO_GPIO23
    PIO_GPIO24
    PIO_GPIO25
    PIO_GPIO26
    PIO_GPIO27
    PIO_GPIO28
    PIO_GPIO29
    PIO_GPIO30
    PIO_GPIO31
    PIO_GPIO32
    PIO_GPIO33
    PIO_GPIO34
    PIO_GPIO35
    PIO_GPIO36
    PIO_GPIO37
    PIO_GPIO38
    PIO_GPIO39
    PIO_GPIO40
    PIO_GPIO41
    PIO_GPIO42
    PIO_GPIO43
    PIO_GPIO44
    PIO_GPIO45
    PIO_GPIO46
    PIO_GPIO47
    PIO_GPIO48
    PIO_GPIO49
    PIO_GPIO50
    PIO_GPIO51
    PIO_GPIO52
    PIO_GPIO53
    PIO_GPIO54
    PIO_GPIO55
    PIO_GPIO56
    PIO_GPIO57
    PIO_GPIO58
    PIO_GPIO59
    PIO_GPIO60
    PIO_GPIO61
    PIO_GPIO62
    PIO_GPIO63
    PIO_GPIO64
    PIO_GPIO65
    PIO_GPIO66
    PIO_GPIO67
    PIO_GPIO68
    PIO_GPIO69
    PIO_GPIO70
    PIO_GPIO71
    PIO_GPIO72
    PIO_GPIO73
    PIO_GPIO74
    PIO_GPIO75
    PIO_GPIO76
    PIO_GPIO77
    PIO_GPIO78
    PIO_GPIO79

    { LNILKEY, LNILVAL }
};
#endif

LUALIB_API int luaopen_pio(lua_State *L) {
    return 0;
}

MODULE_REGISTER_ROM(PIO, pio, pio_map, luaopen_pio, 1);

#endif

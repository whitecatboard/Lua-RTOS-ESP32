#ifndef PIO_H
#define	PIO_H

#include "modules.h"

#include <stdint.h>
#include "sys/drivers/gpio.h"
#include "sys/drivers/cpu.h"

enum
{
  // Pin operations
  PLATFORM_IO_PIN_SET,
  PLATFORM_IO_PIN_CLEAR,
  PLATFORM_IO_PIN_GET,
  PLATFORM_IO_PIN_DIR_INPUT,
  PLATFORM_IO_PIN_DIR_OUTPUT,
  PLATFORM_IO_PIN_PULLUP,
  PLATFORM_IO_PIN_PULLDOWN,
  PLATFORM_IO_PIN_NOPULL,
  PLATFORM_IO_PIN_NUM,
  
  // Port operations
  PLATFORM_IO_PORT_SET_VALUE,
  PLATFORM_IO_PORT_GET_VALUE,
  PLATFORM_IO_PORT_DIR_INPUT,
  PLATFORM_IO_PORT_DIR_OUTPUT
};

#ifdef GPIO1
#define PIO_GPIO1 {LSTRKEY(GPIO1_NAME), LINTVAL(GPIO1)},
#else
#define PIO_GPIO1
#endif

#ifdef GPIO2
#define PIO_GPIO2 {LSTRKEY(GPIO2_NAME), LINTVAL(GPIO2)},
#else
#define PIO_GPIO2
#endif

#ifdef GPIO3
#define PIO_GPIO3 {LSTRKEY(GPIO3_NAME), LINTVAL(GPIO3)},
#else
#define PIO_GPIO3
#endif

#ifdef GPIO4
#define PIO_GPIO4 {LSTRKEY(GPIO4_NAME), LINTVAL(GPIO4)},
#else
#define PIO_GPIO4
#endif

#ifdef GPIO5
#define PIO_GPIO5 {LSTRKEY(GPIO5_NAME), LINTVAL(GPIO5)},
#else
#define PIO_GPIO5
#endif

#ifdef GPIO6
#define PIO_GPIO6 {LSTRKEY(GPIO6_NAME), LINTVAL(GPIO6)},
#else
#define PIO_GPIO6
#endif

#ifdef GPIO7
#define PIO_GPIO7 {LSTRKEY(GPIO7_NAME), LINTVAL(GPIO7)},
#else
#define PIO_GPIO7
#endif

#ifdef GPIO8
#define PIO_GPIO8 {LSTRKEY(GPIO8_NAME), LINTVAL(GPIO8)},
#else
#define PIO_GPIO8
#endif

#ifdef GPIO9
#define PIO_GPIO9 {LSTRKEY(GPIO9_NAME), LINTVAL(GPIO9)},
#else
#define PIO_GPIO9
#endif

#ifdef GPIO10
#define PIO_GPIO10 {LSTRKEY(GPIO10_NAME), LINTVAL(GPIO10)},
#else
#define PIO_GPIO10
#endif

#ifdef GPIO11
#define PIO_GPIO11 {LSTRKEY(GPIO11_NAME), LINTVAL(GPIO11)},
#else
#define PIO_GPIO11
#endif

#ifdef GPIO12
#define PIO_GPIO12 {LSTRKEY(GPIO12_NAME), LINTVAL(GPIO12)},
#else
#define PIO_GPIO12
#endif

#ifdef GPIO13
#define PIO_GPIO13 {LSTRKEY(GPIO13_NAME), LINTVAL(GPIO13)},
#else
#define PIO_GPIO13
#endif

#ifdef GPIO14
#define PIO_GPIO14 {LSTRKEY(GPIO14_NAME), LINTVAL(GPIO14)},
#else
#define PIO_GPIO14
#endif

#ifdef GPIO15
#define PIO_GPIO15 {LSTRKEY(GPIO15_NAME), LINTVAL(GPIO15)},
#else
#define PIO_GPIO15
#endif

#ifdef GPIO16
#define PIO_GPIO16 {LSTRKEY(GPIO16_NAME), LINTVAL(GPIO16)},
#else
#define PIO_GPIO16
#endif


typedef u32_t pio_type;


// Each i/o port are 16 bits - wide


int platform_pio_has_port( unsigned port );
const char* platform_pio_get_prefix( unsigned port );
int platform_pio_has_pin( unsigned port, unsigned pin );
int platform_pio_get_num_pins( unsigned port );
pio_type platform_pio_op( unsigned port, pio_type pinmask, int op );


#endif	


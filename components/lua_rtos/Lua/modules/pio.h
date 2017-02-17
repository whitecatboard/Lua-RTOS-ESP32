#ifndef PIO_H
#define	PIO_H

#include "modules.h"

#include <stdint.h>
#include <drivers/gpio.h>
#include <drivers/cpu.h>

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

#ifdef GPIO0
#define PIO_GPIO0 {LSTRKEY(GPIO0_NAME), LINTVAL(GPIO0)},
#else
#define PIO_GPIO0
#endif

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

#ifdef GPIO17
#define PIO_GPIO17 {LSTRKEY(GPIO17_NAME), LINTVAL(GPIO17)},
#else
#define PIO_GPIO17
#endif

#ifdef GPIO18
#define PIO_GPIO18 {LSTRKEY(GPIO18_NAME), LINTVAL(GPIO18)},
#else
#define PIO_GPIO18
#endif

#ifdef GPIO19
#define PIO_GPIO19 {LSTRKEY(GPIO19_NAME), LINTVAL(GPIO19)},
#else
#define PIO_GPIO19
#endif

#ifdef GPIO20
#define PIO_GPIO20 {LSTRKEY(GPIO20_NAME), LINTVAL(GPIO20)},
#else
#define PIO_GPIO20
#endif

#ifdef GPIO21
#define PIO_GPIO21 {LSTRKEY(GPIO21_NAME), LINTVAL(GPIO21)},
#else
#define PIO_GPIO21
#endif

#ifdef GPIO22
#define PIO_GPIO22 {LSTRKEY(GPIO22_NAME), LINTVAL(GPIO22)},
#else
#define PIO_GPIO22
#endif

#ifdef GPIO23
#define PIO_GPIO23 {LSTRKEY(GPIO23_NAME), LINTVAL(GPIO23)},
#else
#define PIO_GPIO23
#endif

#ifdef GPIO24
#define PIO_GPIO24 {LSTRKEY(GPIO24_NAME), LINTVAL(GPIO24)},
#else
#define PIO_GPIO24
#endif

#ifdef GPIO25
#define PIO_GPIO25 {LSTRKEY(GPIO25_NAME), LINTVAL(GPIO25)},
#else
#define PIO_GPIO25
#endif

#ifdef GPIO26
#define PIO_GPIO26 {LSTRKEY(GPIO26_NAME), LINTVAL(GPIO26)},
#else
#define PIO_GPIO26
#endif

#ifdef GPIO27
#define PIO_GPIO27 {LSTRKEY(GPIO27_NAME), LINTVAL(GPIO27)},
#else
#define PIO_GPIO27
#endif

#ifdef GPIO28
#define PIO_GPIO28 {LSTRKEY(GPIO28_NAME), LINTVAL(GPIO28)},
#else
#define PIO_GPIO28
#endif

#ifdef GPIO29
#define PIO_GPIO29 {LSTRKEY(GPIO29_NAME), LINTVAL(GPIO29)},
#else
#define PIO_GPIO29
#endif

#ifdef GPIO30
#define PIO_GPIO30 {LSTRKEY(GPIO30_NAME), LINTVAL(GPIO30)},
#else
#define PIO_GPIO30
#endif

#ifdef GPIO31
#define PIO_GPIO31 {LSTRKEY(GPIO31_NAME), LINTVAL(GPIO31)},
#else
#define PIO_GPIO31
#endif

#ifdef GPIO32
#define PIO_GPIO32 {LSTRKEY(GPIO32_NAME), LINTVAL(GPIO32)},
#else
#define PIO_GPIO32
#endif

#ifdef GPIO33
#define PIO_GPIO33 {LSTRKEY(GPIO33_NAME), LINTVAL(GPIO33)},
#else
#define PIO_GPIO33
#endif

#ifdef GPIO34
#define PIO_GPIO34 {LSTRKEY(GPIO34_NAME), LINTVAL(GPIO34)},
#else
#define PIO_GPIO34
#endif

#ifdef GPIO35
#define PIO_GPIO35 {LSTRKEY(GPIO35_NAME), LINTVAL(GPIO35)},
#else
#define PIO_GPIO35
#endif

#ifdef GPIO36
#define PIO_GPIO36 {LSTRKEY(GPIO36_NAME), LINTVAL(GPIO36)},
#else
#define PIO_GPIO36
#endif

#ifdef GPIO37
#define PIO_GPIO37 {LSTRKEY(GPIO37_NAME), LINTVAL(GPIO37)},
#else
#define PIO_GPIO37
#endif

#ifdef GPIO38
#define PIO_GPIO38 {LSTRKEY(GPIO38_NAME), LINTVAL(GPIO38)},
#else
#define PIO_GPIO38
#endif

#ifdef GPIO39
#define PIO_GPIO39 {LSTRKEY(GPIO39_NAME), LINTVAL(GPIO39)},
#else
#define PIO_GPIO39
#endif

#ifdef GPIO40
#define PIO_GPIO40 {LSTRKEY(GPIO40_NAME), LINTVAL(GPIO40)},
#else
#define PIO_GPIO40
#endif

#ifdef GPIO41
#define PIO_GPIO41 {LSTRKEY(GPIO41_NAME), LINTVAL(GPIO41)},
#else
#define PIO_GPIO41
#endif

#ifdef GPIO42
#define PIO_GPIO42 {LSTRKEY(GPIO42_NAME), LINTVAL(GPIO42)},
#else
#define PIO_GPIO42
#endif

#ifdef GPIO43
#define PIO_GPIO43 {LSTRKEY(GPIO43_NAME), LINTVAL(GPIO43)},
#else
#define PIO_GPIO43
#endif

#ifdef GPIO44
#define PIO_GPIO44 {LSTRKEY(GPIO44_NAME), LINTVAL(GPIO44)},
#else
#define PIO_GPIO44
#endif

#ifdef GPIO45
#define PIO_GPIO45 {LSTRKEY(GPIO45_NAME), LINTVAL(GPIO45)},
#else
#define PIO_GPIO45
#endif

#ifdef GPIO46
#define PIO_GPIO46 {LSTRKEY(GPIO46_NAME), LINTVAL(GPIO46)},
#else
#define PIO_GPIO46
#endif

int platform_pio_has_port( unsigned port );
const char* platform_pio_get_prefix( unsigned port );
int platform_pio_has_pin( unsigned port, unsigned pin );
int platform_pio_get_num_pins( unsigned port );
gpio_pin_mask_t platform_pio_op( unsigned port, gpio_pin_mask_t pinmask, int op );


#endif	


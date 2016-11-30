#ifndef _LUA_UART_H
#define	_LUA_UART_H

#include "modules.h"

#include <drivers/cpu.h>

#ifdef CPU_UART0
#define UART_UART0 {LSTRKEY(CPU_UART0_NAME), LINTVAL(CPU_UART0)},
#else
#define UART_UART0
#endif

#ifdef CPU_UART1
#define UART_UART1 {LSTRKEY(CPU_UART1_NAME), LINTVAL(CPU_UART1)},
#else
#define UART_UART1
#endif

#ifdef CPU_UART2
#define UART_UART2 {LSTRKEY(CPU_UART2_NAME), LINTVAL(CPU_UART2)},
#else
#define UART_UART2
#endif

#endif

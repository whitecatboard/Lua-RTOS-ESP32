#ifndef LUA_RTOS_LUARTOS_H_
#define LUA_RTOS_LUARTOS_H_

#include "sdkconfig.h"

/*
 *
 * UART
 *
 */

// Use console?
#ifdef CONFIG_USE_CONSOLE
#define USE_CONSOLE CONFIG_USE_CONSOLE
#else
#define USE_CONSOLE 1
#endif

// Get the UART assigned to the console
#if CONFIG_LUA_RTOS_CONSOLE_UART0
#define CONSOLE_UART 1
#endif

#if CONFIG_LUA_RTOS_CONSOLE_UART1
#define CONSOLE_UART 2
#endif

#if CONFIG_LUA_RTOS_CONSOLE_UART2
#define CONSOLE_UART 3
#endif

// Get the console baud rate
#if CONFIG_LUA_RTOS_CONSOLE_BR_57600
#define CONSOLE_BR 57600
#endif

#if CONFIG_LUA_RTOS_CONSOLE_BR_115200
#define CONSOLE_BR 115200
#endif

// Get the console buffer length
#ifdef CONFIG_LUA_RTOS_CONSOLE_BUFFER_LEN
#define CONSOLE_BUFFER_LEN CONFIG_LUA_RTOS_CONSOLE_BUFFER_LEN
#else
#define CONSOLE_BUFFER_LEN 1024
#endif

#ifndef CONSOLE_UART
#define CONSOLE_UART 1
#endif

#ifndef CONSOLE_BR
#define CONSOLE_BR 115200
#endif

#define THREAD_LOCAL_STORAGE_POINTER_ID 0

#endif

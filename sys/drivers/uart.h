#ifndef __UART_H__
#if CONSOLE_SWAP_UART == 0
#define uart0_swap()
#define uart0_default()
#else
void uart0_swap();
void uart0_default();
#endif
#endif

#ifdef PLATFORM_ESP8266
#include <sys/drivers/platform/esp8266/uart.h>
#endif

#ifdef PLATFORM_ESP32
#include <sys/drivers/platform/esp32/uart.h>
#endif

#ifdef PLATFORM_PIC32MZ
#include <sys/drivers/platform/pic32mz/uart.h>
#endif
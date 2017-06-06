#ifndef LUA_RTOS_LUARTOS_H_
#define LUA_RTOS_LUARTOS_H_

#include "freertos/FreeRTOS.h"
#include "esp_task.h"
#include "sdkconfig.h"

/* Board type */
#if CONFIG_LUA_RTOS_BOARD_WHITECAT_N1ESP32
#define LUA_RTOS_BOARD "N1ESP32"
#endif

#if CONFIG_LUA_RTOS_BOARD_ESP32_CORE_BOARD
#define LUA_RTOS_BOARD "ESP32COREBOARD"
#endif

#if CONFIG_LUA_RTOS_BOARD_ESP32_THING
#define LUA_RTOS_BOARD "ESP32THING"
#endif

#if CONFIG_LUA_RTOS_BOARD_OTHER
#define LUA_RTOS_BOARD "GENERIC"
#endif

#ifndef LUA_RTOS_BOARD
#define LUA_RTOS_BOARD "GENERIC"
#endif

/*
 * Lua modules to build
 *
 */
#define USE_NET CONFIG_WIFI_ENABLED || CONFIG_ETHERNET

#define USE_NET_VFS USE_NET

#if CONFIG_LUA_RTOS_USE_HTTP_SERVER
#define LUA_USE_HTTP 1
#else
	#if LUA_RTOS_USE_HTTP_SERVER
	#define LUA_USE_HTTP 1
	#else
	#define LUA_USE_HTTP 0
	#endif
#endif

/*
 * Lua RTOS
 */
#define LUA_TASK_PRIORITY  CONFIG_LUA_RTOS_LUA_TASK_PRIORITY
#define LUA_USE_ROTABLE	   1

#if CONFIG_LUA_RTOS_USE_LED_ACT
#define LED_ACT CONFIG_LUA_RTOS_LED_ACT
#define USE_LED_ACT 1
#else
#define LED_ACT 0
#define USE_LED_ACT 0
#endif

#define SD_LED LED_ACT

/*
 * SPI
 */
#define USE_SPI (CONFIG_LUA_RTOS_LUA_USE_SPI || CONFIG_LUA_RTOS_LUA_USE_LORA)

/*
 * I2C
 */
#define USE_I2C (CONFIG_LUA_RTOS_LUA_USE_I2C)

/*
 * UART
 */
#define USE_UART CONFIG_LUA_RTOS_LUA_USE_UART

// Use console?
#ifdef CONFIG_USE_CONSOLE
#define USE_CONSOLE CONFIG_USE_CONSOLE
#else
#define USE_CONSOLE 1
#endif

// Get the UART assigned to the console
#if CONFIG_LUA_RTOS_CONSOLE_UART0
#define CONSOLE_UART 0
#endif

#if CONFIG_LUA_RTOS_CONSOLE_UART1
#define CONSOLE_UART 1
#endif

#if CONFIG_LUA_RTOS_CONSOLE_UART2
#define CONSOLE_UART 2
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

// LoRa WAN

#define US_PER_OSTICK   20
#define OSTICKS_PER_SEC 50000
#define LMIC_SPI_KHZ    1000

#if CONFIG_LUA_RTOS_LORA_NODE_RADIO_SX1276
	#define CFG_sx1276_radio 1
#else
	#if CONFIG_LUA_RTOS_LORA_NODE_RADIO_SX1272
		#define CFG_sx1272_radio 1
	#else
		#define CFG_sx1276_radio 1
	#endif
#endif

#if CONFIG_LUA_RTOS_LORA_NODE_BAND_EU868
	#define CFG_eu868 1
#else
	#if CONFIG_LUA_RTOS_LORA_NODE_BAND_US915
		#define CFG_us915 1
	#else
		#define CFG_eu868 1
	#endif
#endif

#if CONFIG_FREERTOS_THREAD_LOCAL_STORAGE_POINTERS <= 1
#error "Please, review the 'Number of thread local storage pointers' settings in kconfig. Must be >= 2."
#endif

#define THREAD_LOCAL_STORAGE_POINTER_ID (CONFIG_FREERTOS_THREAD_LOCAL_STORAGE_POINTERS - 1)

#endif

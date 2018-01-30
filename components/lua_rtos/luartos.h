/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS main include file
 *
 */

#ifndef LUA_RTOS_LUARTOS_H_
#define LUA_RTOS_LUARTOS_H_

#include "freertos/FreeRTOS.h"
#include "esp_task.h"
#include "sdkconfig.h"

/* VFS */
/* Calculate how many vfs are needed */
#ifndef CONFIG_SD_CARD_MMC
#define CONFIG_SD_CARD_MMC 0
#endif

#ifndef CONFIG_SD_CARD_SPI
#define CONFIG_SD_CARD_SPI 0
#endif

#ifndef CONFIG_LUA_RTOS_USE_SPIFFS
#define CONFIG_LUA_RTOS_USE_SPIFFS 0
#endif

#ifndef CONFIG_LUA_RTOS_USE_SSH_SERVER
#define CONFIG_LUA_RTOS_USE_SSH_SERVER 0
#endif

#define VFS_COUNT_IDF 3
#define VFS_COUNT_FAT (CONFIG_SD_CARD_MMC + CONFIG_SD_CARD_SPI + 2)
#define VFS_COUNT_SPIFFS (CONFIG_LUA_RTOS_USE_SPIFFS + 1)
#define VFS_COUNT_TTY 1
#define VFS_COUNT_URANDOM CONFIG_LUA_RTOS_USE_SSH_SERVER
#define VFS_COUNT_PTY CONFIG_LUA_RTOS_USE_SSH_SERVER

#define VFS_MAX_COUNT (VFS_COUNT_IDF + VFS_COUNT_FAT + VFS_COUNT_SPIFFS + VFS_COUNT_TTY + VFS_COUNT_URANDOM + VFS_COUNT_PTY)

/* Board type */
#if CONFIG_LUA_RTOS_BOARD_WHITECAT_N1ESP32
#define LUA_RTOS_BOARD "N1ESP32"
#endif

#if CONFIG_LUA_RTOS_BOARD_WHITECAT_N1ESP32_DEVKIT
#define LUA_RTOS_BOARD "N1ESP32-DEVKIT"
#endif


#if CONFIG_LUA_RTOS_BOARD_WHITECAT_ESP32_LORA_GW
#define LUA_RTOS_BOARD "WHITECAT-ESP32-LORA-GW"
#endif

#if CONFIG_LUA_RTOS_BOARD_ESP32_CORE_BOARD
#define LUA_RTOS_BOARD "ESP32COREBOARD"
#endif

#if CONFIG_LUA_RTOS_BOARD_ESP32_WROVER_KIT
#define LUA_RTOS_BOARD "ESP-WROVER-KIT"
#endif

#if CONFIG_LUA_RTOS_BOARD_ESP32_THING
#define LUA_RTOS_BOARD "ESP32THING"
#endif

#if CONFIG_LUA_RTOS_BOARD_ESP32_GATEWAY
#define LUA_RTOS_BOARD "ESP32-GATEWAY"
#endif

#if CONFIG_LUA_RTOS_BOARD_ESP32_EVB
#define LUA_RTOS_BOARD "ESP32-EVB"
#endif

#if CONFIG_LUA_RTOS_BOARD_OTHER
#define LUA_RTOS_BOARD "GENERIC"
#endif

#ifndef LUA_RTOS_BOARD
#define LUA_RTOS_BOARD "GENERIC"
#endif

/*
 * Lua RTOS
 */
#define LUA_USE_ROTABLE	   1

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
#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276
#define CONFIG_LUA_RTOS_LUA_USE_LORA 1
#else
#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272
#define CONFIG_LUA_RTOS_LUA_USE_LORA 1
#else
#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1301
#define CONFIG_LUA_RTOS_LUA_USE_LORA 1
#else
#define CONFIG_LUA_RTOS_LUA_USE_LORA 0
#endif
#endif
#endif

#define US_PER_OSTICK   20
#define OSTICKS_PER_SEC 50000
#define LMIC_SPI_KHZ    1000

#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276
	#define CFG_sx1276_radio 1
#else
	#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272
		#define CFG_sx1272_radio 1
	#else
		#define CFG_sx1276_radio 1
	#endif
#endif

#if CONFIG_LUA_RTOS_LORA_BAND_EU868
	#define CFG_eu868 1
#else
	#if CONFIG_LUA_RTOS_LORA_BAND_US915
		#define CFG_us915 1
	#else
		#define CFG_eu868 1
	#endif
#endif

#if CONFIG_FREERTOS_THREAD_LOCAL_STORAGE_POINTERS <= 1
#error "Please, review the 'Number of thread local storage pointers' settings in kconfig. Must be >= 2."
#endif

#define THREAD_LOCAL_STORAGE_POINTER_ID (CONFIG_FREERTOS_THREAD_LOCAL_STORAGE_POINTERS - 1)

// External GPIO
#define EXTERNAL_GPIO 0
#define EXTERNAL_GPIO_PINS 0
#define EXTERNAL_GPIO_PORTS 0
#if CONFIG_GPIO_PCA9698 ||  CONFIG_GPIO_PCA9505
	#undef EXTERNAL_GPIO_PINS
	#define EXTERNAL_GPIO_PINS 40
	#undef EXTERNAL_GPIO_PORTS
	#define EXTERNAL_GPIO_PORTS 5
	#undef EXTERNAL_GPIO
	#define EXTERNAL_GPIO 1

	#if CONFIG_GPIO_PCA9698
	#define EXTERNAL_GPIO_NAME "PCA9698"
	#endif

#if CONFIG_GPIO_PCA9505
	#define EXTERNAL_GPIO_NAME "PCA9505"
	#endif
#endif

// OpenVPN
#if CONFIG_LUA_RTOS_USE_OPENVPN
#if !CONFIG_MBEDTLS_BLOWFISH_C
#error "OpenVPM requires CONFIG_MBEDTLS_BLOWFISH_C = 1. Please activate it with make menuconfig, enabling option in mbedTLS -> Symmetric Ciphers -> Blowfish block cipher."
#endif
#endif

#if __has_include("luartos_custom.h")
#include "luartos_custom.h"
#endif

#endif

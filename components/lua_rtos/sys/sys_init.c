/*
 * Lua RTOS, system init
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "sdkconfig.h"
#include "luartos.h"
#include "lua.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_sleep.h"
#include "esp_ota_ops.h"

#include "driver/periph_ctrl.h"

#include "nvs_flash.h"
#include "nvs.h"

#include <esp_spi_flash.h>

#include <string.h>
#include <stdio.h>

#include <vfs/vfs.h>
#include <sys/reent.h>
#include <sys/syslog.h>
#include <sys/console.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/driver.h>
#include <sys/delay.h>
#include <sys/status.h>

#include <drivers/cpu.h>
#include <drivers/uart.h>

extern void _pthread_init();
extern void _signal_init();
extern void _cpu_init();
extern void _clock_init();

extern const char *__progname;

// Boot count
RTC_DATA_ATTR uint32_t boot_count = 0;

// Flash unique id
uint8_t flash_unique_id[8];

#ifdef RUN_TESTS
#include <unity.h>

#include <pthread.h>

void *_sys_tests(void *arg) {
	printf("Running tests ...\r\n\r\n");

	unity_run_all_tests();

	printf("\r\nTests done!");

	for(;;);

	pthread_exit(NULL);
}

#endif

/*
   Shows the firmware copyright notice. You can modify the default copyright notice if
   the following conditions are met:

   1. The whitecat logo cannot be changed. You can remove the whitecat logo, but you
      cannot change it. The whitecat logo is:

        /\       /\
       /  \_____/  \
      /_____________\
      W H I T E C A T

   2. Any other copyright notices cannot be removed. This includes any references to
      Lua RTOS, Lua, and copyright notices that may appear in the future.
*/
void __attribute__((weak)) firmware_copyright_notice() {
	printf("  /\\       /\\\r\n");
    printf(" /  \\_____/  \\\r\n");
    printf("/_____________\\\r\n");
    printf("W H I T E C A T\r\n\r\n");
}

void _sys_init() {
	nvs_flash_init();

	#if CONFIG_LUA_RTOS_ETH_HW_TYPE_RMII
	#if CONFIG_PHY_POWER_PIN > 0
		//gpio_pin_output(CONFIG_PHY_POWER_PIN);
		//gpio_pin_clr(CONFIG_PHY_POWER_PIN);
	#endif
	#endif

	// Set default power down mode for all RTC power domains in deep sleep
	#if CONFIG_LUA_RTOS_DEEP_SLEEP_RTC_PERIPH
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
	#else
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
	#endif

	#if CONFIG_LUA_RTOS_DEEP_SLEEP_RTC_SLOW_MEM
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
	#else
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
	#endif

	#if CONFIG_LUA_RTOS_DEEP_SLEEP_RTC_FAST_MEM
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
	#else
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
	#endif

	// Increment bootcount
	boot_count++;

	esp_log_level_set("*", ESP_LOG_ERROR);

	// Disable hardware modules modules
	periph_module_disable(PERIPH_LEDC_MODULE);

	// set the current time only if RTC has not already been set
	struct timeval tv;
	gettimeofday(&tv, NULL);
	if (tv.tv_sec < BUILD_TIME) {
		tv.tv_sec = BUILD_TIME;
		tv.tv_usec = 0;
		settimeofday(&tv, NULL);
	}

	#if CONFIG_LUA_RTOS_READ_FLASH_UNIQUE_ID
	// Get flash unique id
	uint8_t command[13] = {0x4b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t response[13];

	spi_flash_send_cmd(sizeof(command), command, response);
	memcpy(flash_unique_id, response + 5, sizeof(flash_unique_id));
	#endif

	// Init important things for Lua RTOS
	_clock_init();
	_cpu_init();
    _driver_init();
    _pthread_init();

    status_set(STATUS_SYSCALLS_INITED);
    status_set(STATUS_LUA_SHELL);
    status_set(STATUS_LUA_HISTORY);

    _signal_init();

    esp_vfs_lwip_sockets_register();

	esp_vfs_unregister("/dev/uart");
	esp_vfs_unregister("/dev/uart");

	vfs_tty_register();

#if CONFIG_LUA_RTOS_USE_SSH_SERVER
	vfs_pty_register();
	vfs_urandom_register();
#endif

	printf("Booting Lua RTOS...\r\n");
	delay(100);

	console_clear();

	firmware_copyright_notice();

    const esp_partition_t *running = esp_ota_get_running_partition();

    printf(
		"Lua RTOS %s. Copyright (C) 2015 - 2017 whitecatboard.org\r\n\r\nbuild %d\r\ncommit %s\r\nRunning from %s partition\r\n",
		LUA_OS_VER, BUILD_TIME, BUILD_COMMIT, running->label
	);

    printf("board type %s\r\n", LUA_RTOS_BOARD);

	#ifdef RUN_TESTS
		// Create and run a pthread for tests
		pthread_attr_t attr;
		struct sched_param sched;
		pthread_t thread;
		int res;

		// Init thread attributes
		pthread_attr_init(&attr);

		// Set stack size
	    pthread_attr_setstacksize(&attr, CONFIG_LUA_RTOS_LUA_STACK_SIZE);

	    // Set priority
	    sched.sched_priority = CONFIG_LUA_RTOS_LUA_TASK_PRIORITY;
	    pthread_attr_setschedparam(&attr, &sched);

	    // Set CPU
	    cpu_set_t cpu_set = CPU_INITIALIZER;

	    CPU_SET(CONFIG_LUA_TASK_CPU, &cpu_set);

	    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);

		// Create thread
		res = pthread_create(&thread, &attr, _sys_tests, NULL);
		if (res) {
			panic("Cannot start tests");
		}

		vTaskDelete(NULL);
	#endif

    openlog(__progname, LOG_CONS | LOG_NDELAY, LOG_LOCAL1);

    cpu_show_info();
    cpu_show_flash_info();

    //Init filesystems
    #if CONFIG_LUA_RTOS_USE_SPIFFS
    	vfs_spiffs_register();
    #endif

	#if CONFIG_SD_CARD_MMC || CONFIG_SD_CARD_SPI
    	vfs_fat_register();

    	if (mount_is_mounted("fat")) {
            // Redirect console messages to /log/messages.log ...
            closelog();
            syslog(LOG_INFO, "redirecting console messages to file system ...");
            openlog(__progname, LOG_NDELAY , LOG_LOCAL1);
        } else {
        	syslog(LOG_ERR, "can't redirect console messages to file system, an SDCARD is needed");
        }
    #endif

    // Continue init ...
    printf("\n");
}

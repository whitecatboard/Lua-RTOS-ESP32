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
 * Lua RTOS system init
 *
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
extern void _cpu_init();
extern void _clock_init();
extern void _signal_init();
extern void _status_init();
extern void vfs_tty_register();

// Boot count
RTC_DATA_ATTR uint32_t boot_count = 0;

#if CONFIG_LUA_RTOS_READ_FLASH_UNIQUE_ID
// Flash unique id
uint8_t flash_unique_id[8];
#endif

#ifdef RUN_TESTS
#include <unity.h>

#include <pthread.h>

void testTask(void *pvParameters) {
    vTaskDelay(2);

    printf("Running tests ...\r\n\r\n");

    unity_run_all_tests();

    printf("\r\nTests done!");

    vTaskDelete(NULL);
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

    // Disable hardware modules modules
    periph_module_disable(PERIPH_LEDC_MODULE);
    periph_module_disable(PERIPH_CAN_MODULE);
    periph_module_disable(PERIPH_I2C0_MODULE);
    periph_module_disable(PERIPH_I2C1_MODULE);
    periph_module_disable(PERIPH_RMT_MODULE);

    // Init important things for Lua RTOS
    _mount_init();
    _status_init();
    _clock_init();
    _cpu_init();
    _driver_init();
    _signal_init();

    status_set(STATUS_SYSCALLS_INITED, 0x00000000);
    status_set(STATUS_LUA_SHELL, 0x00000000);
    status_set(STATUS_LUA_HISTORY, 0x00000000);

    esp_vfs_lwip_sockets_register();

    vfs_tty_register();

    printf("Booting Lua RTOS...\r\n");
    delay(100);

    console_clear();

    firmware_copyright_notice();

    const esp_partition_t *running = esp_ota_get_running_partition();

    printf(
        "Lua RTOS %s. Copyright (C) 2015 - 2018 whitecatboard.org\r\n\r\nbuild %d\r\ncommit %s\r\nRunning from %s partition\r\n",
        LUA_OS_VER, BUILD_TIME, BUILD_COMMIT, running->label
    );

    printf("board type %s\r\n", CONFIG_LUA_RTOS_BOARD_TYPE);

    // Open log
#if CONFIG_LOG_DEFAULT_LEVEL_NONE
    setlogmask(LOG_UPTO(LOG_EMERG));
#elif CONFIG_LOG_DEFAULT_LEVEL_ERROR
    setlogmask(LOG_UPTO(LOG_ERR));
#elif CONFIG_LOG_DEFAULT_LEVEL_WARN
    setlogmask(LOG_UPTO(LOG_WARNING));
#elif CONFIG_LOG_DEFAULT_LEVEL_INFO
    setlogmask(LOG_UPTO(LOG_INFO));
#elif CONFIG_LOG_DEFAULT_LEVEL_DEBUG
    setlogmask(LOG_UPTO(LOG_DEBUG));
#elif CONFIG_LOG_DEFAULT_LEVEL_VERBOSE
    setlogmask(LOG_UPTO(LOG_DEBUG));
#endif
    openlog(LOG_CONS | LOG_NDELAY, LOG_LOCAL1);

    cpu_show_info();
    cpu_show_flash_info();

    // Mount the root file system
    mount("/", CONFIG_LUA_RTOS_ROOT_FS);

    // After mounting the file systems, open log again, to allow log messages redirection
    // to messages.log file.
    closelog();
    openlog(LOG_CONS | LOG_NDELAY, LOG_LOCAL1);

#ifdef RUN_TESTS
    xTaskCreatePinnedToCore(testTask, "testTask", 8192, NULL, UNITY_FREERTOS_PRIORITY, NULL, UNITY_FREERTOS_CPU);
    vTaskDelete(NULL);
#endif

    // Continue init ...
    printf("\n");
}

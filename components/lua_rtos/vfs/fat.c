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
 * Lua RTOS fat vfs
 *
 */

#include "sdkconfig.h"

#if CONFIG_SD_CARD_MMC || CONFIG_SD_CARD_SPI

#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_defs.h"

#include <sys/driver.h>
#include <sys/mount.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>

extern spi_bus_t spi_bus[CPU_LAST_SPI - CPU_FIRST_SPI + 1];

void vfs_fat_register() {
#if CONFIG_SD_CARD_SPI

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	// Lock resources
	if (spi_lock_bus_resources(CONFIG_LUA_RTOS_SD_SPI, DRIVER_ALL_FLAGS)) {
		return;
	}

	if (driver_lock(SYSTEM_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_SD_CS, DRIVER_ALL_FLAGS, "SD Card - CS")) {
		return;
	}
#endif

	#if (CONFIG_LUA_RTOS_SD_SPI == 2)
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    host.slot = HSPI_HOST;

    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();

    slot_config.gpio_miso = CONFIG_LUA_RTOS_SPI2_MISO;
    slot_config.gpio_mosi = CONFIG_LUA_RTOS_SPI2_MOSI;
    slot_config.gpio_sck  = CONFIG_LUA_RTOS_SPI2_CLK;
    slot_config.gpio_cs   = CONFIG_LUA_RTOS_SD_CS;
	#endif

	#if (CONFIG_LUA_RTOS_SD_SPI == 3)
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    host.slot = VSPI_HOST;

    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();

    slot_config.gpio_miso = CONFIG_LUA_RTOS_SPI3_MISO;
    slot_config.gpio_mosi = CONFIG_LUA_RTOS_SPI3_MOSI;
    slot_config.gpio_sck  = CONFIG_LUA_RTOS_SPI3_CLK;
    slot_config.gpio_cs   = CONFIG_LUA_RTOS_SD_CS;
	#endif

    spi_bus[spi_idx(CONFIG_LUA_RTOS_SD_SPI)].setup |= SPI_DMA_SETUP;
#endif

#if CONFIG_SD_CARD_MMC
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

	#if CONFIG_LUA_RTOS_MCC_1_LINE
    host.flags = SDMMC_HOST_FLAG_1BIT;
	#endif

    slot_config.gpio_cd = CONFIG_LUA_RTOS_MCC_CD;
    slot_config.gpio_wp = CONFIG_LUA_RTOS_MCC_WP;

    // Lock resources
	if (driver_lock(SYSTEM_DRIVER, 0, GPIO_DRIVER, 15, DRIVER_ALL_FLAGS, "SD Card - CMD")) {
		return;
	}

	if (driver_lock(SYSTEM_DRIVER, 0, GPIO_DRIVER, 14, DRIVER_ALL_FLAGS, "SD Card - CLK")) {
		return;
	}

	if (driver_lock(SYSTEM_DRIVER, 0, GPIO_DRIVER, 2, DRIVER_ALL_FLAGS, "SD Card - DAT0")) {
		return;
	}

#if !CONFIG_LUA_RTOS_MCC_1_LINE
	if (driver_lock(SYSTEM_DRIVER, 0, GPIO_DRIVER, 14, DRIVER_ALL_FLAGS, "SD Card - DAT1")) {
		return;
	}

	if (driver_lock(SYSTEM_DRIVER, 0, GPIO_DRIVER, 12, DRIVER_ALL_FLAGS, "SD Card - DAT2")) {
		return;
	}
#endif

#if CONFIG_LUA_RTOS_MCC_CD != -1
	if (driver_lock(SYSTEM_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_MCC_CD, DRIVER_ALL_FLAGS, "SD Card - CD")) {
		return;
	}
#endif

#if CONFIG_LUA_RTOS_MCC_WP != -1
	if (driver_lock(SYSTEM_DRIVER, 0, GPIO_DRIVER, CONFIG_LUA_RTOS_MCC_WP, DRIVER_ALL_FLAGS, "SD Card - WP")) {
		return;
	}
#endif

	slot_config.width = 4;
#endif

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

#if CONFIG_SD_CARD_SPI
    syslog(LOG_INFO, "sd%u is at spi%d, cs=%s%d", 0,
			CONFIG_LUA_RTOS_SD_SPI,
			gpio_portname(CONFIG_LUA_RTOS_SD_CS), gpio_name(CONFIG_LUA_RTOS_SD_CS)
	);
#endif

#if CONFIG_SD_CARD_MMC
    syslog(LOG_INFO, "sd%u is at mmc0", 0);
#endif


    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/fat", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
    	esp_vfs_fat_sdmmc_unmount();
    	syslog(LOG_INFO, "fat%d can't mounted", 0);
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    	driver_unlock_all(SYSTEM_DRIVER, 0);
#endif

#if CONFIG_SD_CARD_SPI
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
   	#if (CONFIG_LUA_RTOS_SD_SPI == 2)
    spi_unlock_bus_resources(2);
	#endif

	#if (CONFIG_LUA_RTOS_SD_SPI == 3)
	spi_unlock_bus_resources(3);
	#endif
#endif
#endif
    	return;
    }

	syslog(LOG_INFO, "sd%u name %s", 0, card->cid.name);
	syslog(LOG_INFO, "sd%u type %s", 0, (card->ocr & SD_OCR_SDHC_CAP)?"SDHC/SDXC":"SDSC");
	syslog(LOG_INFO, "sd%u speed %s", 0, (card->csd.tr_speed > 25000000)?"high speed":"default speed");
	syslog(LOG_INFO, "sd%u size %.2f GB", 0,
		((((double)card->csd.capacity) * ((double)card->csd.sector_size)) / 1073741824.0)
	);

	syslog(LOG_INFO, "sd%u CSD ver=%d, sector_size=%d, capacity=%d read_bl_len=%d", 0,
		card->csd.csd_ver,
		card->csd.sector_size, card->csd.capacity, card->csd.read_block_len
	);

	syslog(LOG_INFO, "sd%u SCR sd_spec=%d, bus_width=%d", 0, card->scr.sd_spec, card->scr.bus_width);

    mount_set_mounted("fat", 1);

    syslog(LOG_INFO, "fat%d mounted", 0);
}

void vfs_fat_format() {
}

#endif

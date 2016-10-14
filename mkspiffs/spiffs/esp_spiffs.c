/**
 * ESP8266 SPIFFS HAL configuration.
 *
 * Part of esp-open-rtos
 * Copyright (c) 2016 sheinz https://github.com/sheinz
 * MIT License
 */
#include "esp_spiffs.h"
#include "spiffs.h"
#include <espressif/spi_flash.h>
#include "esp_spiffs_flash.h"

s32_t esp_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
    if (esp_spiffs_flash_read(addr, dst, size) == ESP_SPIFFS_FLASH_ERROR) {
        return SPIFFS_ERR_INTERNAL;
    }

    return SPIFFS_OK;
}

s32_t esp_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
    if (esp_spiffs_flash_write(addr, src, size) == ESP_SPIFFS_FLASH_ERROR) {
        return SPIFFS_ERR_INTERNAL;
    }

    return SPIFFS_OK;
}

s32_t esp_spiffs_erase(u32_t addr, u32_t size)
{
    uint32_t sectors = size / SPI_FLASH_SEC_SIZE;

    for (uint32_t i = 0; i < sectors; i++) {
        if (esp_spiffs_flash_erase_sector(addr + (SPI_FLASH_SEC_SIZE * i))
                == ESP_SPIFFS_FLASH_ERROR) {
            return SPIFFS_ERR_INTERNAL;
        }
    }

    return SPIFFS_OK;
}
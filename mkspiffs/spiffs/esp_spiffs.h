/**
 * ESP8266 SPIFFS HAL configuration.
 *
 * Part of esp-open-rtos
 * Copyright (c) 2016 sheinz https://github.com/sheinz
 * MIT License
 */
#ifndef __ESP_SPIFFS_H__
#define __ESP_SPIFFS_H__

#include "spiffs.h"

s32_t esp_spiffs_read(u32_t addr, u32_t size, u8_t *dst);
s32_t esp_spiffs_write(u32_t addr, u32_t size, u8_t *src);
s32_t esp_spiffs_erase(u32_t addr, u32_t size);

#endif  // __ESP_SPIFFS_H__

#ifndef __PIC32MZ_SPIFFS_H__
#define __PIC32MZ_SPIFFS_H__

#include "../../spiffs.h"

#include <stdint.h>

s32_t my_spiffs_read(u32_t addr, u32_t size, u8_t *dst);
s32_t my_spiffs_write(u32_t addr, u32_t size, u8_t *src);
s32_t my_spiffs_erase(u32_t addr, u32_t size);

#define low_spiffs_read my_spiffs_read
#define low_spiffs_write my_spiffs_write
#define low_spiffs_erase my_spiffs_erase

#endif  // __PIC32MZ_SPIFFS_H__

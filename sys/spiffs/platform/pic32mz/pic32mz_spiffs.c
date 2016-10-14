#include "../../spiffs.h"
#include "pic32mz_spiffs.h"

#include <sys/drivers/cfi.h>

s32_t my_spiffs_read(u32_t addr, u32_t size, u8_t *dst) {
    cfi_read(0, addr, size, (char *)dst);
    return SPIFFS_OK;
}

s32_t my_spiffs_write(u32_t addr, u32_t size, u8_t *src) {
    cfi_write(0, addr, size, (char *)src);
    return SPIFFS_OK;
}

s32_t my_spiffs_erase(u32_t addr, u32_t size) {
    cfi_erase(0, addr, size);
    return SPIFFS_OK;
}
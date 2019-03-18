#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "utilities.h"

static portMUX_TYPE critical_sect_mux = portMUX_INITIALIZER_UNLOCKED;

void BoardCriticalSectionBegin(uint32_t *mask) {
    portENTER_CRITICAL(&critical_sect_mux);
}

void BoardCriticalSectionEnd(uint32_t *mask) {
    portEXIT_CRITICAL(&critical_sect_mux);
}

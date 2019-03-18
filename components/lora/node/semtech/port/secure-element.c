#include "secure-element.h"
#include "esp_system.h"

#include <stdint.h>

SecureElementStatus_t SecureElementRandomNumber( uint32_t* randomNum ) {
    *randomNum = esp_random();

    return SECURE_ELEMENT_SUCCESS;
}

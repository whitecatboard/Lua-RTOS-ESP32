#include "freertos/FreeRTOS.h"

#include "adc.h"
#include "eeprom-board.h"
#include "gpio.h"
#include "i2c.h"
#include "rtc-board.h"
#include "spi.h"
#include "timer.h"
#include "uart.h"
#include "utilities.h"

#if defined(SX1261MBXBAS) || defined(SX1262MBXCAS) || defined(SX1262MBXDAS)
#include "sx126x-board.h"
#elif defined(LR1110MB1XXS)
#include "lr1110-board.h"
#elif defined(SX1272MB2DAS)
#include "sx1272-board.h"
#elif defined(SX1276MB1LAS) || defined(SX1276MB1MAS)
#include "sx1276-board.h"
#endif

#include "board.h"

#include "esp_flash.h"
#include "spi_flash_mmap.h"

#define AUTO 0

extern void EepromMcuInit(void);

// External reference to FLASH UNIQUE ID
extern uint8_t flash_unique_id[8];

// Spinlock to control critical regions
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

/*!
 * Flag to indicate if the MCU is Initialized
 */
static bool McuInitialized = false;

void BoardCriticalSectionBegin(uint32_t *mask) { portENTER_CRITICAL(&spinlock); }

void BoardCriticalSectionEnd(uint32_t *mask) { portEXIT_CRITICAL(&spinlock); }

void BoardInitPeriph(void) {}

void BoardInitMcu(void) {
  if (McuInitialized == true) {
    return;
  }

  EepromMcuInit();
  RtcInit();

#if defined(SX1261MBXBAS) || defined(SX1262MBXCAS) || defined(SX1262MBXDAS)
  SpiInit(&SX1276.Spi, CONFIG_LUA_RTOS_LORA_SPI, AUTO, AUTO, AUTO, CONFIG_LUA_RTOS_LORA_CS);
  SX126xIoInit();
#elif defined(LR1110MB1XXS)
  SpiInit(&SX1276.Spi, CONFIG_LUA_RTOS_LORA_SPI, AUTO, AUTO, AUTO, CONFIG_LUA_RTOS_LORA_CS);
  lr1110_board_init_io(&LR1110);
#elif defined(SX1272MB2DAS)
  SpiInit(&SX1276.Spi, CONFIG_LUA_RTOS_LORA_SPI, AUTO, AUTO, AUTO, CONFIG_LUA_RTOS_LORA_CS);
  SX1272IoInit();
#elif defined(SX1276MB1LAS) || defined(SX1276MB1MAS)
  SpiInit(&SX1276.Spi, CONFIG_LUA_RTOS_LORA_SPI, AUTO, AUTO, AUTO, CONFIG_LUA_RTOS_LORA_CS);
  SX1276IoInit();
#endif

  McuInitialized = true;
}

void BoardResetMcu(void) {
}

void BoardDeInitMcu(void) {
}

uint32_t BoardGetRandomSeed(void) {
#if 0
    return ( ( *( uint32_t* )ID1 ) ^ ( *( uint32_t* )ID2 ) ^ ( *( uint32_t* )ID3 ) );
#else
  return 0;
#endif
}

void BoardGetUniqueId(uint8_t *id) { esp_flash_read_unique_chip_id(NULL, (uint64_t *)id); }
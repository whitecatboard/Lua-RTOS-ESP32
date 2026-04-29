/*!
 * \file      eeprom-board.c
 *
 * \brief     Target board EEPROM driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */

#include "eeprom_flash.h"

#include "eeprom-board.h"
#include "utilities.h"
#include <stdbool.h>
#include <stdint.h>

void EepromMcuInit(void) {  
  driver_error_log_and_destroy(eeprom_init());
}

bool EepromMcuIsErasingOnGoing(void) { return false; }

LmnStatus_t EepromMcuWriteBuffer(uint16_t addr, uint8_t *buffer, uint16_t size) {
  driver_error_t *error = eeprom_write(addr, buffer, size);
  if (error) {
    driver_error_log_and_destroy(error);
	return LMN_STATUS_ERROR;
  }
  
  return LMN_STATUS_OK;
}

LmnStatus_t EepromMcuReadBuffer(uint16_t addr, uint8_t *buffer, uint16_t size) {
  driver_error_t *error = eeprom_read(addr, buffer, size);
  if (error) {
    driver_error_log_and_destroy(error);
    return LMN_STATUS_ERROR;
  }

  return LMN_STATUS_OK;
}

void EepromMcuSetDeviceAddr(uint8_t addr) { }

LmnStatus_t EepromMcuGetDeviceAddr(void) {
  return LMN_STATUS_ERROR;
}
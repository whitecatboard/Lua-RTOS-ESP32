/*
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 * Copyright (C) 2015 - 2026, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2026, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Lua RTOS, EEPROM flash emulation driver
 */
 
#ifndef SYS_DRIVERS_EEPROM_FLASH_H_
#define SYS_DRIVERS_EEPROM_FLASH_H_

#include <stdint.h>
#include <sys/driver.h>

// Size of the page size used in the implementation in bytes. Can be either 4, 8, 16,
// 32, 64, 128, or 256 bytes.
#ifndef EEPROM_PAGE_SIZE
#define EEPROM_PAGE_SIZE 128
#endif

// EEPROM errors
#define EEPROM_ERR_INVALID_PARTITION             (DRIVER_EXCEPTION_BASE(EEPROM_DRIVER_ID) |  0)
#define EEPROM_ERR_NO_MEM                        (DRIVER_EXCEPTION_BASE(EEPROM_DRIVER_ID) |  1)
#define EEPROM_ERR_FLASH_ACCESS_ERROR            (DRIVER_EXCEPTION_BASE(EEPROM_DRIVER_ID) |  2)
#define EEPROM_ERR_FLASH_INTEGRITY_ERROR         (DRIVER_EXCEPTION_BASE(EEPROM_DRIVER_ID) |  3)
#define EEPROM_ERR_NO_SPACE_LEFT                 (DRIVER_EXCEPTION_BASE(EEPROM_DRIVER_ID) |  4)

extern const int eeprom_errors;
extern const int eeprom_error_map;

/**
 * @brief Performs consistency recovery and mounts the EEPROM partition.
 *
 * Acts as the recovery engine. It analyzes bank headers and page states to 
 * identify the 'Source of Truth'. If a power failure occurs during a bank swap 
 * or GC, it uses bank states and wear-leveling counters to repair the partition.
 *
 * @return NULL on success, or a driver_error_t pointer if recovery fails.
 */
driver_error_t *eeprom_init(void);

/**
 * @brief Reads data from a logical EEPROM address.
 *
 * Abstracts the log-structured flash by mapping the logical address to its 
 * most recent physical location in the active bank.
 *
 * @param[in] addr Starting logical address.
 * @param[out] dest Destination buffer.
 * @param[in] size Number of bytes to read.
 * @return NULL on success, or a driver_error_t pointer.
 */
driver_error_t *eeprom_read(uint32_t address, void *dst, uint32_t size);

/**
 * @brief Performs an atomic transactional write to the EEPROM.
 *
 * Implements a "look-ahead" capacity check. If the write crosses a bank boundary, 
 * it triggers a GC event to enter shadow-paging mode. This ensures multi-page 
 * writes are never split between banks, maintaining atomicity during power loss.
 *
 * @param[in] addr Starting logical address.
 * @param[in] src Source buffer.
 * @param[in] size Number of bytes to write.
 * @return NULL on success, or a driver_error_t pointer.
 */
 driver_error_t *eeprom_write(uint32_t address, void *src, size_t size);

 /**
  * @brief Performs a structural re-initialization of the EEPROM storage.
  *
  * Resets the entire partition, clears all data, and re-establishes base 
  * metadata signatures and wear-leveling counters.
  *
  * @return NULL on success, or a driver_error_t pointer.
  */
driver_error_t *eeprom_erase(void);

#endif /* SYS_DRIVERS_EEPROM_FLASH_H_ */

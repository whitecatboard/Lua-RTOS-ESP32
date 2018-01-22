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
 * Lua RTOS, SPI driver
 *
 */

/*
 * This driver is inspired and takes code from the following projects:
 *
 * arduino-esp32 (https://github.com/espressif/arduino-esp32)
 * esp32-nesemu (https://github.com/espressif/esp32-nesemu
 * esp-open-rtos (https://github.com/SuperHouse/esp-open-rtos)
 *
 * By default low level access uses Lua RTOS implementation, instead of spi_master from esp-idf that uses DMA transfers.
 * You can turn on esp-idf use setting SPI_USE_IDF_DRIVER to 1. Actually seems that spi_master from esp-idf have some
 * performance issues (although uses DMA) and issues related to read operations using DMA.
 *
 * Initial work in this driver was made by the Lua RTOS team in the espi driver (see):
 *
 * https://github.com/whitecatboard/Lua-RTOS-ESP32/commit/e4cfeccf60ddd2301c137537b3f8e039d3762869#diff-03afb94387bf851f6050a3066103cc67
 *
 * Work in espi driver was continued by Boris Lovošević (see):
 *
 * https://github.com/loboris/Lua-RTOS-ESP32-lobo/commit/1da097fd4e4c5bca61c28ea2f03bee17c84942f5#diff-03afb94387bf851f6050a3066103cc67
 *
 * Finally the Lua RTOS team have integrated all the ideas on the espi driver in the same driver.
 *
 */

#ifndef _SPI_H_
#define _SPI_H_

#include "driver/spi_master.h"

#include <sys/driver.h>

// Number of SPI devices per bus
#define SPI_BUS_DEVICES 3

// Get the index for a SPI unit in the spi_bus array
#define spi_idx(unit) (unit - CPU_FIRST_SPI)

// Check if SPI unit use the native pins
#define spi_use_native_pins(unit) \
	( \
		(unit==2)?((spi_bus[spi_idx(unit)].miso == 12) && (spi_bus[spi_idx(unit)].mosi == 13) && (spi_bus[spi_idx(unit)].clk == 14)): \
		((spi_bus[spi_idx(unit)].miso == 19) && (spi_bus[spi_idx(unit)].mosi == 23) && (spi_bus[spi_idx(unit)].clk == 18)) \
	)

// Native pins
#define SPI_DEFAULT_MISO(unit) (unit==2?GPIO12:(unit==3?GPIO19:-1))
#define SPI_DEFAULT_MOSI(unit) (unit==2?GPIO13:(unit==3?GPIO23:-1))
#define SPI_DEFAULT_CLK(unit)  (unit==2?GPIO14:(unit==3?GPIO18:-1))

// SPI errors
#define SPI_ERR_INVALID_MODE             (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  0)
#define SPI_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  1)
#define SPI_ERR_SLAVE_NOT_ALLOWED	 	 (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  2)
#define SPI_ERR_NOT_ENOUGH_MEMORY	 	 (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  3)
#define SPI_ERR_PIN_NOT_ALLOWED		     (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  4)
#define SPI_ERR_NO_MORE_DEVICES_ALLOWED  (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  5)
#define SPI_ERR_INVALID_DEVICE 			 (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  6)
#define SPI_ERR_DEVICE_NOT_SETUP     	 (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  7)
#define SPI_ERR_DEVICE_IS_NOT_SELECTED 	 (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  8)
#define SPI_ERR_CANNOT_CHANGE_PINMAP 	 (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  9)

extern const int spi_errors;
extern const int spi_error_map;

#define SPI_DMA_SETUP    (1 << 0)
#define SPI_NO_DMA_SETUP (1 << 1)

// Flags
#define SPI_FLAG_WRITE  (1 << 0)
#define SPI_FLAG_READ   (1 << 1)
#define SPI_FLAG_NO_DMA (1 << 2)
#define SPI_FLAG_3WIRE  (1 << 3)

typedef struct {
	uint8_t  setup;
	int8_t   cs;
	uint8_t  mode;
	uint8_t  dma;
	uint32_t regs[14];
	spi_device_handle_t h;
} spi_device_t;

typedef struct {
	SemaphoreHandle_t mtx; // Recursive mutex for access the bus
	uint8_t setup;         // Bus is setup?
	int last_device;       // Last device that used the bus
	int selected_device;   // Device that owns the bus

    // Current pin assignment
	int8_t miso;
	int8_t mosi;
	int8_t clk;

	// Spi devices attached to the bus
	spi_device_t device[SPI_BUS_DEVICES];
} spi_bus_t;

/**
 * @brief Select SPI device for start a transaction over the SPI bus to the device. This function is thread safe.
 *        You must call to this function prior to any write / read operation with the device.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 *
 */
void spi_ll_select(int deviceid);

/**
 * @brief Deselect SPI device for end the transaction over the SPI bus to the device. This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 *
 */
void spi_ll_deselect(int deviceid);

void spi_ll_unsetup(int deviceid);

/**
 * @brief Get SPI device speed in Hertz. This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param speed A pointer to an integer variable where device speed must be set.
 *
 */
void spi_ll_get_speed(int deviceid, uint32_t *speed);

/**
 * @brief Set SPI device speed in Hertz. This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param speed Device speed in Hertz.
 *
 */
void spi_ll_set_speed(int deviceid, uint32_t speed);

/**
 * @brief Transfer and read 1 byte of data to / from the device. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param data Byte to transfer
 * @param read A pointer to a byte to set the readed data.
 *
 */
void spi_ll_transfer(int deviceid, uint8_t data, uint8_t *read);

/**
 * @brief Transfer a chunk of bytes to the device. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param nbytes Number of bytes to transfer.
 * @param data A pointer to the buffer to transfer.
 *
 */
void spi_ll_bulk_write(int deviceid, uint32_t nbytes, uint8_t *data);

/**
 * @brief Read a chunk of bytes from the device. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param nbytes Number of bytes to read.
 * @param data A pointer to the buffer to store the read data.
 *
 */
void spi_ll_bulk_read(int deviceid, uint32_t nbytes, uint8_t *data);

/**
 * @brief Transfer and read a chunk of bytes to / from the device. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param nbytes Number of bytes to read.
 * @param data A pointer to the buffer with the data to transfer. Read data are stored to
 *        this buffer.
 *
 * @return -1 if not enough memory, 0 if ok
 */
int spi_ll_bulk_rw(int deviceid, uint32_t nbytes, uint8_t *data);

/**
 * @brief Transfer a chunk of 16-bit data to the device. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to transfer.
 * @param data A pointer to the buffer to transfer.
 *
 */
void spi_ll_bulk_write16(int deviceid, uint32_t nelements, uint16_t *data);

/**
 * @brief Read a chunk of 16-bit data from the device. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to read.
 * @param data A pointer to the buffer to store the read data.
 *
 */
void spi_ll_bulk_read16(int deviceid, uint32_t nelements, uint16_t *data);

/**
 * @brief Transfer and read a chunk 16-bit data to / from the device. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to read.
 * @param data A pointer to the buffer with the data to transfer. Read data are stored to
 *        this buffer.
 *
 * @return -1 if not enough memory, 0 if ok
 */
int spi_ll_bulk_rw16(int deviceid, uint32_t nelements, uint16_t *data);

/**
 * @brief Transfer a chunk of 32-bit data to the device. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to transfer.
 * @param data A pointer to the buffer to transfer.
 *
 */
void spi_ll_bulk_write32(int deviceid, uint32_t nelements, uint32_t *data);

/**
 * @brief Read a chunk of 32-bit data from the device. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to read.
 * @param data A pointer to the buffer to store the read data.
 *
 */
void spi_ll_bulk_read32(int deviceid, uint32_t nelements, uint32_t *data);

/**
 * @brief Transfer and read a chunk 32-bit data to / from the device. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to transfer / read.
 * @param data A pointer to the buffer with the data to transfer. Read data are stored to
 *        this buffer.
 *
 * @return -1 if not enough memory, 0 if ok
 */
int spi_ll_bulk_rw32(int deviceid, uint32_t nelements, uint32_t *data);

/**
 * @brief Change the SPI pin map. Pin map is hard coded in Kconfig, but it can be
 *        change in development environments. This function is thread safe.
 *
 * @param unit SPI unit. 2 = HSPI, 3 = VSPI.
 * @param miso MISO signal gpio number.
 * @param mosi MOSI signal gpio number.
 * @param clk CLK signal gpio number.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs. Error can be an operation error or a lock error.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_PIN_NOT_ALLOWED
 *     	 SPI_ERR_CANNOT_CHANGE_PINMAP
 */
driver_error_t *spi_pin_map(int unit, int miso, int mosi, int clk);

/**
 * @brief Setup SPI device attached to a SPI bus. This function is thread safe.
 *
 * @param unit SPI unit. 2 = HSPI, 3 = VSPI.
 * @param master 1 for master mode, 0 for slave mode (not supported for now).
 * @param cs GPIO number for the cs signal.
 * @param mode SPI mode, can be either 0, 1, 2 or 3.
 * @param speed Speed in Hertz.
 * @param flags. A mask formed by one of the following SPI_FLAG_WRITE, SPI_FLAG_READ.
 * @param deviceid A pointer to an integer with a device identifier assigned to the SPI device.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs. Error can be an operation error or a lock error.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_SLAVE_NOT_ALLOWED
 *     	 SPI_ERR_INVALID_MODE
 *     	 SPI_ERR_PIN_NOT_ALLOWED
 *     	 SPI_ERR_NO_MORE_DEVICES_ALLOWED
 */
driver_error_t *spi_setup(uint8_t unit, uint8_t master, int8_t cs, uint8_t mode, uint32_t speed, uint8_t flags, int *deviceid);

driver_error_t *spi_unsetup(int deviceid);

/**
 * @brief Select SPI device for start a transaction over the SPI bus to the device. This function is thread safe.
 *        You must call to this function prior to any write / read operation with the device.
 *
 * @param deviceid Device identifier.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_NOT_SETUP
 */
driver_error_t *spi_select(int deviceid);

/**
 * @brief Deselect SPI device for end the transaction over the SPI bus to the device. This function is thread safe.
 *
 * @param deviceid Device identifier.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_NOT_SETUP
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 */
driver_error_t *spi_deselect(int deviceid);

/**
 * @brief Get SPI device speed in Hertz. This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param speed A pointer to an integer variable where device speed must be set.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_NOT_SETUP
 */
driver_error_t *spi_get_speed(int deviceid, uint32_t *speed);

/**
 * @brief Set SPI device speed in Hertz. This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param speed Device speed in Hertz.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_NOT_SETUP
 */
driver_error_t *spi_set_speed(int deviceid, uint32_t speed);

/**
 * @brief Transfer and read 1 byte of data to / from the device. Device must be selected
 *        before calling this function (use spi_select for that). This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param data Byte to transfer
 * @param read A pointer to a byte to set the readed data.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 */
driver_error_t *spi_transfer(int deviceid, uint8_t data, uint8_t *read);

/**
 * @brief Transfer a chunk of bytes to the device. Device must be selected
 *        before calling this function (use spi_select for that). This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param nbytes Number of bytes to transfer.
 * @param data A pointer to the buffer to transfer.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 */
driver_error_t *spi_bulk_write(int deviceid, uint32_t nbytes, uint8_t *data);

/**
 * @brief Read a chunk of bytes from the device. Device must be selected
 *        before calling this function (use spi_select for that). This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param nbytes Number of bytes to read.
 * @param data A pointer to the buffer to store the read data.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 */
driver_error_t *spi_bulk_read(int deviceid, uint32_t nbytes, uint8_t *data);

/**
 * @brief Transfer and read a chunk of bytes to / from the device. Device must be selected
 *        before calling this function (use spi_select for that). This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param nbytes Number of bytes to read.
 * @param data A pointer to the buffer with the data to transfer. Read data are stored to
 *        this buffer.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 *     	 SPI_ERR_NOT_ENOUGH_MEMORY
 */
driver_error_t *spi_bulk_rw(int deviceid, uint32_t nbytes, uint8_t *data);

/**
 * @brief Transfer a chunk of 16-bit data to the device. Device must be selected
 *        before calling this function (use spi_select for that). This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to transfer.
 * @param data A pointer to the buffer to transfer.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 */
driver_error_t *spi_bulk_write16(int deviceid, uint32_t nelements, uint16_t *data);

/**
 * @brief Read a chunk of 16-bit data from the device. Device must be selected
 *        before calling this function (use spi_select for that). This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to read.
 * @param data A pointer to the buffer to store the read data.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 */
driver_error_t *spi_bulk_read16(int deviceid, uint32_t nelements, uint16_t *data);

/**
 * @brief Transfer and read a chunk 16-bit data to / from the device. Device must be selected
 *        before calling this function (use spi_select for that). This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to read.
 * @param data A pointer to the buffer with the data to transfer. Read data are stored to
 *        this buffer.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 *     	 SPI_ERR_NOT_ENOUGH_MEMORY
 */
driver_error_t *spi_bulk_rw16(int deviceid, uint32_t nelements, uint16_t *data);

/**
 * @brief Transfer a chunk of 32-bit data to the device. Device must be selected
 *        before calling this function (use spi_select for that). This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to transfer.
 * @param data A pointer to the buffer to transfer.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 */
driver_error_t *spi_bulk_write32(int deviceid, uint32_t nelements, uint32_t *data);

/**
 * @brief Read a chunk of 32-bit data from the device. Device must be selected
 *        before calling this function (use spi_select for that). This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to read.
 * @param data A pointer to the buffer to store the read data.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 */
driver_error_t *spi_bulk_read32(int deviceid, uint32_t nelements, uint32_t *data);

/**
 * @brief Transfer and read a chunk 32-bit data to / from the device. Device must be selected
 *        before calling this function (use spi_select for that). This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to transfer / read.
 * @param data A pointer to the buffer with the data to transfer. Read data are stored to
 *        this buffer.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 SPI_ERR_INVALID_DEVICE
 *     	 SPI_ERR_DEVICE_IS_NOT_SELECTED
 *     	 SPI_ERR_NOT_ENOUGH_MEMORY
 */
driver_error_t *spi_bulk_rw32(int deviceid, uint32_t nelements, uint32_t *data);

driver_error_t *spi_lock_bus_resources(int unit, uint8_t flags);
void spi_unlock_bus_resources(int unit);

#endif

/*
 * Lua RTOS, SPI driver
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 *
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#ifndef _SPI_H_
#define _SPI_H_

#include <sys/driver.h>

// Number of SPI devices per bus
#define SPI_BUS_DEVICES 3

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
#define SPI_ERR_INVALID_FLAG 	         (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  9)

// Flags
#define SPI_FLAG_WRITE 0x01
#define SPI_FLAG_READ  0x02
#define SPI_FLAG_ALL (SPI_FLAG_WRITE | SPI_FLAG_READ)

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
 * @brief Transfer a chunk of 32-bit data to the device. Data bust be transfer in big endian. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to transfer.
 * @param data A pointer to the buffer to transfer.
 *
 */
void spi_ll_bulk_write32_be(int deviceid, uint32_t nelements, uint32_t *data);

/**
 * @brief Read a chunk of 32-bit data from the device. Data bust be transfer in big endian. Device must be selected
 *        before calling this function (use spi_ll_select for that). This function is thread safe.
 *        No sanity checks are done (use only in driver develop).
 *
 * @param deviceid Device identifier.
 * @param nelements Number of elements to read.
 * @param data A pointer to the buffer to store the read data.
 *
 */
void spi_ll_bulk_read32_be(int deviceid, uint32_t nelements, uint32_t *data);

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
driver_error_t *spi_setup(uint8_t unit, uint8_t master, uint8_t cs, uint8_t mode, uint32_t speed, uint8_t flags, int *deviceid);

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

/**
 * @brief Transfer a chunk of 32-bit data to the device. Data bust be transfer in big endian. Device must be selected
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
driver_error_t *spi_bulk_write32_be(int deviceid, uint32_t nelements, uint32_t *data);

/**
 * @brief Read a chunk of 32-bit data from the device. Data bust be transfer in big endian. Device must be selected
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
driver_error_t *spi_bulk_read32_be(int deviceid, uint32_t nelements, uint32_t *data);

#endif

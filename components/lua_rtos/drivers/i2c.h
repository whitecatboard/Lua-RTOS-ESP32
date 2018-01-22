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
 * Lua RTOS, I2C driver
 *
 */

#include "sdkconfig.h"

#ifndef I2C_H
#define I2C_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/i2c.h"

#include <stdint.h>

#include <sys/driver.h>

#include <drivers/cpu.h>

#define I2C_BUS_DEVICES 3
#define I2C_TRANSACTION_INITIALIZER -1

typedef struct i2c_device {
	int speed;
} i2c_device_t;

// Internal driver structure
typedef struct i2c {
	uint8_t mode;
	uint8_t setup;
	int8_t sda;
	int8_t scl;
	SemaphoreHandle_t mtx;
	i2c_device_t device[I2C_BUS_DEVICES];
} i2c_t;

#define I2C_SLAVE	0 /*!< I2C slave mode */
#define I2C_MASTER	1 /*!< I2C master mode */

// I2C errors
#define I2C_ERR_CANT_INIT                (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  0)
#define I2C_ERR_IS_NOT_SETUP             (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  1)
#define I2C_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  2)
#define I2C_ERR_INVALID_OPERATION		 (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  3)
#define I2C_ERR_NOT_ENOUGH_MEMORY		 (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  4)
#define I2C_ERR_INVALID_TRANSACTION		 (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  5)
#define I2C_ERR_NOT_ACK					 (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  6)
#define I2C_ERR_TIMEOUT					 (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  7)
#define I2C_ERR_PIN_NOT_ALLOWED		     (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  8)
#define I2C_ERR_CANNOT_CHANGE_PINMAP     (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  10)
#define I2C_ERR_NO_MORE_DEVICES_ALLOWED  (DRIVER_EXCEPTION_BASE(I2C_DRIVER_ID) |  11)
extern const int i2c_errors;
extern const int i2c_error_map;

/**
 * @brief Change the I2C pin map. Pin map is hard coded in Kconfig, but it can be
 *        change in development environments. This function is thread safe.
 *
 * @param unit I2C unit, can be either 0 or 1.
 * @param sda SDA signal gpio number.
 * @param sdl SCL signal gpio number.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 I2C_ERR_PIN_NOT_ALLOWED
 */
driver_error_t *i2c_pin_map(int unit, int sda, int scl);

/**
 * @brief Setup I2C device attached to a I2C bus. This function is thread safe.
 *
 * @param unit I2C unit, can be either 0 or 1.
 * @param mode I2C mode, can be either I2C_MASTER or I2C_SLAVE.
 * @param speed I2C bus speed, expressed in kilohertzs.
 * @param addr10_en In slave mode, if 1 enables 10-bit address, if 0 disables 10-bit address.
 * @param addr In slave mode, the device address.
 * @param deviceid A pointer to an integer with a device identifier assigned to the I2C device.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 A LOCK error, if sda, or scl gpios are used by other driver
 */
driver_error_t *i2c_setup(int unit, int mode, int speed, int addr10_en, int addr, int *deviceid);

driver_error_t *i2c_setspeed(int unit, int speed);

/**
 * @brief Start an I2C transaction, if configured in master mode. This function is thread safe.
 *        The transaction stores all the I2C in a buffer until i2c_stop or i2c_flush is called.
 *
 * @param deviceid Device identifier.
 * @param transaction A pointer to an integer used to store the transaction's id.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 I2C_ERR_INVALID_UNIT
 *     	 I2C_ERR_IS_NOT_SETUP
 *     	 I2C_ERR_INVALID_OPERATION
 */
driver_error_t *i2c_start(int deviceid, int *transaction);

/**
 * @brief Stop an I2C transaction, if configured in master mode. This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param transaction A pointer to an integer which stores the transaction's id.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 I2C_ERR_INVALID_UNIT
 *     	 I2C_ERR_IS_NOT_SETUP
 *     	 I2C_ERR_INVALID_OPERATION
 *     	 I2C_ERR_INVALID_TRANSACTION
 */
driver_error_t *i2c_stop(int deviceid, int *transaction);

/**
 * @brief Write an adress for read, or write, if configured in master mode. This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param transaction A pointer to an integer which stores the transaction's id.
 * @param adress The address.
 * @param read Can be either 0 (write) or 1 (read).
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 I2C_ERR_INVALID_UNIT
 *     	 I2C_ERR_IS_NOT_SETUP
 *     	 I2C_ERR_INVALID_OPERATION
 *     	 I2C_ERR_INVALID_TRANSACTION
 */
driver_error_t *i2c_write_address(int deviceid, int *transaction, char address, int read);

/**
 * @brief Write data, if configured in master mode. This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param transaction A pointer to an integer which stores the transaction's id.
 * @param data A pointer to the data buffer to send.
 * @param len Length of data to send, in bytes.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 I2C_ERR_INVALID_UNIT
 *     	 I2C_ERR_IS_NOT_SETUP
 *     	 I2C_ERR_INVALID_OPERATION
 *     	 I2C_ERR_INVALID_TRANSACTION
 */
driver_error_t *i2c_write(int deviceid, int *transaction, char *data, int len);

/**
 * @brief Read data, if configured in master mode. This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param transaction A pointer to an integer which stores the transaction's id.
 * @param data A pointer to the data buffer to read.
 * @param len Length of data to read, in bytes.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 I2C_ERR_INVALID_UNIT
 *     	 I2C_ERR_IS_NOT_SETUP
 *     	 I2C_ERR_INVALID_OPERATION
 *     	 I2C_ERR_INVALID_TRANSACTION
 */
driver_error_t *i2c_read(int deviceid, int *transaction, char *data, int len);

/**
 * @brief Flush all operations. This function is thread safe.
 *
 * @param deviceid Device identifier.
 * @param transaction A pointer to an integer which stores the transaction's id.
 * @param new_transaction If 1 creates a new transaction.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 I2C_ERR_INVALID_UNIT
 *     	 I2C_ERR_IS_NOT_SETUP
 *     	 I2C_ERR_INVALID_OPERATION
 *     	 I2C_ERR_INVALID_TRANSACTION
 */
driver_error_t *i2c_flush(int deviceid, int *transaction, int new_transaction);

#endif /* I2C_H */

/*
 * Lua RTOS, I2C driver
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#include "sdkconfig.h"

#ifndef I2C_H
#define I2C_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/i2c.h"

#include <stdint.h>

#include <sys/driver.h>

#include <drivers/cpu.h>

#define I2C_TRANSACTION_INITIALIZER -1

// Internal driver structure
typedef struct i2c {
	uint8_t mode;
	uint8_t setup;
	int8_t sda;
	int8_t scl;
	SemaphoreHandle_t mtx;
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
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 SPI_ERR_INVALID_UNIT
 *     	 A LOCK error, if sda, or scl gpios are used by other driver
 */
driver_error_t *i2c_setup(int unit, int mode, int speed, int addr10_en, int addr);

driver_error_t *i2c_setspeed(int unit, int speed);

/**
 * @brief Start an I2C transaction, if configured in master mode. This function is thread safe.
 *        The transaction stores all the I2C in a buffer until i2c_stop or i2c_flush is called.
 *
 * @param unit I2C unit, can be either 0 or 1.
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
driver_error_t *i2c_start(int unit, int *transaction);

/**
 * @brief Stop an I2C transaction, if configured in master mode. This function is thread safe.
 *
 * @param unit I2C unit, can be either 0 or 1.
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
driver_error_t *i2c_stop(int unit, int *transaction);

/**
 * @brief Write an adress for read, or write, if configured in master mode. This function is thread safe.
 *
 * @param unit I2C unit, can be either 0 or 1.
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
driver_error_t *i2c_write_address(int unit, int *transaction, char address, int read);

/**
 * @brief Write data, if configured in master mode. This function is thread safe.
 *
 * @param unit I2C unit, can be either 0 or 1.
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
driver_error_t *i2c_write(int unit, int *transaction, char *data, int len);

/**
 * @brief Read data, if configured in master mode. This function is thread safe.
 *
 * @param unit I2C unit, can be either 0 or 1.
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
driver_error_t *i2c_read(int unit, int *transaction, char *data, int len);

/**
 * @brief Flush all operations. This function is thread safe.
 *
 * @param unit I2C unit, can be either 0 or 1.
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
driver_error_t *i2c_flush(int unit, int *transaction, int new_transaction);

#endif /* I2C_H */

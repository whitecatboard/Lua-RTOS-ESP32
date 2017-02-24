/*
 * Lua RTOS, Enhanced SPI driver
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

#if 0

#ifndef _ESPI_H_
#define _ESPI_H_

#include <sys/driver.h>

// SPI errors
#define ESPI_ERR_INVALID_MODE             (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  0)
#define ESPI_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  1)
#define ESPI_ERR_SLAVE_NOT_ALLOWED	 	  (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  2)
#define ESPI_ERR_NOT_ENOUGH_MEMORY	 	  (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  3)
#define ESPI_ERR_NO_MORE_DEVICES_ALLOWED  (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  4)
#define ESPI_ERR_PIN_NOT_ALLOWED		  (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  5)
#define ESPI_ERR_CANNOT_UPDATE_PIN_MAP    (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  6)

/**
 * @brief  Update the pin map related to a SPI unit. At the initial state the pin map are
 *         initialized with the native values. If you need to change the pin map call this
 *         function prior calling any espi_xxx functions.
 *
 * @param  unit SPI unit number
 * @param  miso GPIO assigned to the miso signal
 * @param  mosi GPIO assigned to the mosi signal
 * @param  clk GPIO assigned to the clk signal
 *
 * @return
 *     - NULL  On success
 *     - A pointer to a driver_error_t structure, with information about the error
 *
 */
driver_error_t *espi_update_pin_map(uint8_t unit, uint8_t miso, uint8_t mosi, uint8_t clk);

/**
 * @brief  Setup a SPI unit device.
 *
 * @param  unit SPI unit number
 * @param  master Configure unit as master or slave (for now only master)
 * @param  cs GPIO assigned to the device's cs signal
 * @param  mode SPI mode, can be either 0, 1, 2 or 3
 * @param  speed Speed in hertzs
 * @param  deviceid Pointer to a variable to hold the device id assigned
 *
 * @return
 *     - NULL  On success
 *     - A pointer to a driver_error_t structure, with information about the error
 *
 */
driver_error_t *espi_setup(uint8_t unit, uint8_t master, uint8_t cs, uint8_t mode, uint32_t speed, int *deviceid);

driver_error_t *espi_transfer(int deviceid, uint8_t in, uint8_t *out);

#endif

#endif

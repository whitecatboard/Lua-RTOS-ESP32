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

// Resources used by the SPI
typedef struct {
	uint8_t sdi;
	uint8_t sdo;
	uint8_t sck;
	uint8_t cs;
} spi_resources_t;


// SPI errors
#define SPI_ERR_INVALID_MODE             (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  0)
#define SPI_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  1)
#define SPI_ERR_SLAVE_NOT_ALLOWED	 	 (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  2)
#define SPI_ERR_NOT_ENOUGH_MEMORY	 	 (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  3)
#define SPI_ERR_PIN_NOT_ALLOWED		     (DRIVER_EXCEPTION_BASE(SPI_DRIVER_ID) |  4)

void spi_master_op(int unit, unsigned int word_size, unsigned int len, unsigned char *out, unsigned char *in);

driver_error_t *spi_init(int unit, int master);
driver_error_t *spi_setup(int unit);
driver_error_t *spi_set_speed(int unit, unsigned int khz);
driver_error_t *spi_set_cspin(int unit, int pin);
driver_error_t *spi_select(int unit);
driver_error_t *spi_deselect(int unit);

const char *spi_name(int unit);
int spi_cs_gpio(int unit);
void spi_pin_config(int unit, unsigned char sdi, unsigned char sdo, unsigned char sck, unsigned char cs);

unsigned int spi_get_speed(int unit);

driver_error_t *spi_set_mode(int unit, int mode);

driver_error_t *spi_transfer(int unit, unsigned int data, unsigned char *read);
driver_error_t *spi_bulk_write(int unit, unsigned int nbytes, unsigned char *data);
driver_error_t *spi_bulk_read(int unit, unsigned int nbytes, unsigned char *data);
driver_error_t *spi_bulk_rw(int unit, unsigned int nbytes, unsigned char *data);
driver_error_t *spi_bulk_write16(int unit, unsigned int nelem, short *data);
driver_error_t *spi_bulk_read16(int unit, unsigned int nelem, short *data);
driver_error_t *spi_bulk_rw16(int unit, unsigned int nelem, short *data);
driver_error_t *spi_bulk_write32(int unit, unsigned int nelem, int *data);
driver_error_t *spi_bulk_write32_be(int unit, unsigned int nelem, int *data);
driver_error_t *spi_bulk_read32_be(int unit, unsigned int nelem, int *data);

#endif

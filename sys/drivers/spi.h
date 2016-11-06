/*
 * Lua RTOS, SPI driver
 *
 * Copyright (C) 2015 - 2016
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

#ifndef __ROOT_SPI_H__
#define __ROOT_SPI_H__

int spi_init(int unit);

/*
 * Setup SPI connection on a given port (0..5) with a specified chip select pin.
 * Use default speed.
 * Return 0 if the SPI port is not configured.
 */
int spi_setup(int unit);

/*
 * Set the SPI bit rate for a device (in kHz).
 */
void spi_set_speed(int unit, unsigned int khz);

/*
 * Setup the chip select pin for the SPI device.
 * Chip select pin is encoded as 0bPPPPNNNN, or 0xPN, where:
 * N is the pin number 0..F,
 * P is the port index 1..A:
 *   1 - port A
 *   2 - port B
 *   3 - port C
 *   4 - port D
 *   5 - port E
 *   6 - port F
 *   7 - port G
 *   8 - port H
 *   9 - port J
 *   A - port K
 */
void spi_set_cspin(int unit, int pin);


/*
 * Assert the CS pin of a device.
 */
void spi_select(int unit);

/*
 * Deassert the CS pin of a device.
 */
void spi_deselect(int unit);

/*
 * Set a mode setting or two - just updates the internal records,
 * the actual mode is changed next time the CS is asserted.
 */
void spi_set(int unit, unsigned int set);

void spi_clr_and_set(int unit, unsigned int set);

/*
 * Clear a mode setting or two - just updates the internal records,
 * the actual mode is changed next time the CS is asserted.
 */
void spi_clr(int unit, unsigned set);

/*
 * Return the current status of the SPI bus for the device in question.
 * Just returns the ->stat entry in the register set.
 */
unsigned int spi_status(int unit);

/*
 * Return the name of the SPI bus for a device.
 */
const char *spi_name(int unit);

/*
 * Return the port name (A-K) of the chip select pin for a device.
 */
const char *spi_csname(int unit);

/*
 * Return the pin index of the chip select pin for a device.
 */
int spi_cspin(int unit);

/*
 * Return the speed in kHz.
 */
unsigned int spi_get_speed(int unit);

/*
 * Transfer one word of data, and return the read word of data.
 * The actual number of bits sent depends on the mode of the transfer.
 * This is blocking, and waits for the transfer to complete
 * before returning.  Times out after a certain period.
 */
unsigned int spi_transfer(int unit, unsigned int data);

/*
 * Transmit a chunk of 8-bit data.
 */
void spi_bulk_write(int unit, unsigned int nbytes, unsigned char *data);
void spi_bulk_read(int unit, unsigned int nbytes, unsigned char *data);
void spi_bulk_rw(int unit, unsigned int nbytes, unsigned char *data);

/*
 * Transmit a chunk of 16-bit data.
 */
void spi_bulk_write16(int unit, unsigned int nelem, short *data);
void spi_bulk_read16(int unit, unsigned int nelem, short *data);
void spi_bulk_rw16(int unit, unsigned int nelem, short *data);

/*
 * Transmit a chunk of 32-bit data.
 */
void spi_bulk_write32(int unit, unsigned int nelem, int *data);
void spi_bulk_read32(int unit, unsigned int nelem, int *data);
void spi_bulk_rw32(int unit, unsigned int nelem, int *data);

/*
 * Transmit a chunk of 32-bit data with reversed endianness.
 */
void spi_bulk_write32_be(int unit, unsigned int nelem, int *data);
void spi_bulk_read32_be(int unit, unsigned int nelem, int *data);
void spi_bulk_rw32_be(int unit, unsigned int nelem, int *data);


// Include platform depend definitions

#ifdef PLATFORM_ESP8266
#endif

#ifdef PLATFORM_ESP32
#include <sys/drivers/platform/esp32/spi.h>
#endif

#ifdef PLATFORM_PIC32MZ
#include <sys/drivers/platform/pic32mz/spi.h>
#endif

#endif

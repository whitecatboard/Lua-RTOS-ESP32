/*
 * SD flash card disk driver.
 *
 * Copyright (C) 2014 Serge Vakulenko, <serge@vak.ru>
 *
 * -------------------------------------------------------------
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Lua RTOS integration
 *
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * -------------------------------------------------------------
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

#ifndef SD_H
#define SD_H

#include <sys/driver.h>

#define NSD             1
#define NPARTITIONS     4
#define SECTSIZE        512

// SD SPI errors
#define SPI_SD_ERR_CANT_INIT             (DRIVER_EXCEPTION_BASE(SPI_SD_DRIVER_ID) |  0)

driver_error_t *sd_init(int unit);

int sd_size(int unit);
int sd_write(int unit, unsigned offset, char *data, unsigned bcount);
int sd_read(int unit, unsigned int offset, char *data, unsigned int bcount);
int sd_has_partition(int unit, int type);
int sd_has_partitions(int unit);

#endif

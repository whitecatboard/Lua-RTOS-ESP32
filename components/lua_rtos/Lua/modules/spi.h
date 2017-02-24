/*
 * Lua RTOS, SPI wrapper
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

#ifndef LSPI_H
#define	LSPI_H

#include "luartos.h"

#include <drivers/spi.h>
#include <drivers/cpu.h>

typedef struct {
    unsigned char spi;
    unsigned char cs;
    unsigned int  speed;
    unsigned int  mode;
    unsigned int  bits;
} spi_userdata;

#ifdef CPU_SPI0
#define SPI_SPI0 {LSTRKEY(CPU_SPI0_NAME), LINTVAL(CPU_SPI0)},
#else
#define SPI_SPI0
#endif

#ifdef CPU_SPI1
#define SPI_SPI1 {LSTRKEY(CPU_SPI1_NAME), LINTVAL(CPU_SPI1)},
#else
#define SPI_SPI1
#endif

#ifdef CPU_SPI2
#define SPI_SPI2 {LSTRKEY(CPU_SPI2_NAME), LINTVAL(CPU_SPI2)},
#else
#define SPI_SPI2
#endif

#ifdef CPU_SPI3
#define SPI_SPI3 {LSTRKEY(CPU_SPI3_NAME), LINTVAL(CPU_SPI3)},
#else
#define SPI_SPI3
#endif

#endif

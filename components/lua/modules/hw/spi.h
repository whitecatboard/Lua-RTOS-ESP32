/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, Lua SPI module
 *
 */
#ifndef LSPI_H
#define	LSPI_H

#include "luartos.h"

#include <drivers/spi.h>
#include <drivers/cpu.h>

typedef struct {
	int spi_device; // SPI device
	uint8_t  *buff; // Data buffer
	uint32_t len;   // Data buffer length
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

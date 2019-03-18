/*!
 * \file      spi-board.c
 *
 * \brief     Target board SPI driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */

#include "sdkconfig.h"

#include "board.h"
#include "gpio.h"
#include "spi-board.h"

#include <sys/drivers/spi.h>

static int spi_device;

void SpiInit( Spi_t *obj, SpiId_t spiId, PinNames mosi, PinNames miso, PinNames sclk, PinNames nss )
{
    obj->SpiId = CONFIG_LUA_RTOS_LORA_SPI;

    obj->Mosi.pin = mosi;
    obj->Miso.pin = miso;
    obj->Sclk.pin = sclk;
    obj->Nss.pin = nss;

    // Setup SPI
    driver_error_t *error;

    if ((error = spi_setup(obj->SpiId, 1, obj->Nss.pin, 0, 1000000, SPI_FLAG_WRITE | SPI_FLAG_READ | SPI_FLAG_NO_DMA, &spi_device))) {
    	// TO DO
    }
}

void SpiDeInit( Spi_t *obj )
{
	driver_error_t *error;

	if ((error = spi_unsetup(spi_device))) {
    	// TO DO
	}
}

uint16_t SpiInOut( Spi_t *obj, uint16_t outData )
{
    driver_error_t *error;
    uint8_t rxData = 0;

    if ((error = spi_transfer(spi_device, (uint8_t)outData, &rxData))) {
    }

    return( rxData );
}

int SpiDevice(Spi_t *obj) {
	return spi_device;
}

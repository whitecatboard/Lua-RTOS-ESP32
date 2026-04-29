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
#include "utilities.h"
#include "board.h"
#include "gpio.h"
#include "spi-board.h"

#include <drivers/spi.h>

static int deviceid;

void SpiInit( Spi_t *obj, SpiId_t spiId, PinNames mosi, PinNames miso, PinNames sclk, PinNames nss )
{
    obj->SpiId = spiId;

    obj->Mosi.pin = mosi;
    obj->Miso.pin = miso;
    obj->Sclk.pin = sclk;
    obj->Nss.pin = nss;

    // Setup SPI
    driver_error_t *error;

    if ((error = spi_setup(obj->SpiId, 1, obj->Nss.pin, 0, 1000000, SPI_FLAG_WRITE | SPI_FLAG_READ | SPI_FLAG_NO_DMA, &deviceid))) {
		goto driver_error;
    }
    
    return;
    	
driver_error:
	driver_error_log_and_destroy(error);    
}

void SpiDeInit( Spi_t *obj )
{
	driver_error_t *error;

	if ((error = spi_unsetup(deviceid))) {
		goto driver_error;
    }
    	
    return;
    
driver_error:
	driver_error_log_and_destroy(error);    
}

uint16_t SpiInOut( Spi_t *obj, uint16_t outData )
{
    driver_error_t *error;
    uint8_t rxData = 0;

    if ((error = spi_transfer(deviceid, (uint8_t)outData, &rxData))) {
		goto driver_error;
    }

    return( rxData );
    	
driver_error:
	driver_error_log_and_destroy(error);
	
	return 0;    
}

int SpiGetDevice(void) {
	return deviceid;
}


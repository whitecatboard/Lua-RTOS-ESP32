#ifndef LSPI_H
#define	LSPI_H

#include "drivers/spi/spi.h"

typedef struct {
    unsigned char spi;
    unsigned char cs;
    unsigned int speed;
    unsigned int mode;
} spi_userdata;

typedef u32 spi_data_type;

#endif

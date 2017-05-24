/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech-Cycleo

Description:
    Host specific functions to address the LoRa concentrator registers through
    a SPI interface.
    Single-byte read/write and burst read/write.
    Does not handle pagination.
    Could be used with multiple SPI ports in parallel (explicit file descriptor)

License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Sylvain Miermont
*/

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LORA_DEVICE_TYPE_GATEWAY

#define SPI_DELAY() delay(1)

#include <drivers/spi.h>

/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>        /* C99 types */
#include <stdio.h>        /* printf fprintf */
#include <stdlib.h>        /* malloc free */
#include <unistd.h>        /* lseek, close */
#include <fcntl.h>        /* open */
#include <string.h>        /* memset */

#include "loragw_spi.h"
#include "loragw_hal.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#if DEBUG_SPI == 1
    #define DEBUG_MSG(str)                fprintf(stderr, str)
    #define DEBUG_PRINTF(fmt, args...)    fprintf(stderr,"%s:%d: "fmt, __FUNCTION__, __LINE__, args)
    #define CHECK_NULL(a)                if(a==NULL){fprintf(stderr,"%s:%d: ERROR: NULL POINTER AS ARGUMENT\n", __FUNCTION__, __LINE__);return LGW_SPI_ERROR;}
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, args...)
    #define CHECK_NULL(a)                if(a==NULL){return LGW_SPI_ERROR;}
#endif

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

#define READ_ACCESS     0x00
#define WRITE_ACCESS    0x80
#define SPI_SPEED       8000000

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

/* SPI initialization and configuration */
int lgw_spi_open(void **spi_target_ptr) {
    int *spi_device = NULL;
    int dev;

    /* check input variables */
    CHECK_NULL(spi_target_ptr); /* cannot be null, must point on a void pointer (*spi_target_ptr can be null) */

    /* allocate memory for the device descriptor */
    spi_device = malloc(sizeof(int));
    if (spi_device == NULL) {
        DEBUG_MSG("ERROR: MALLOC FAIL\n");
        return LGW_SPI_ERROR;
    }

    /* open SPI device */
    driver_error_t *error;

    if ((error = spi_setup(CONFIG_LUA_RTOS_LORA_GATEWAY_SPI, 1, CONFIG_LUA_RTOS_LORA_GATEWAY_CS, 0, CONFIG_LUA_RTOS_LORA_GATEWAY_SPEED, SPI_FLAG_WRITE | SPI_FLAG_READ, &dev))) {
        DEBUG_PRINTF("ERROR: failed to open SPI device SPI%d\n", CONFIG_LUA_RTOS_LORA_GATEWAY_SPI);
        return LGW_SPI_ERROR;
    }

    *spi_device = dev;
    *spi_target_ptr = (void *)spi_device;
    DEBUG_MSG("Note: SPI port opened and configured ok\n");
    return LGW_SPI_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* SPI release */
int lgw_spi_close(void *spi_target) {
	DEBUG_MSG("Note: SPI port closed\n");
	return LGW_SPI_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Simple write */
int lgw_spi_w(void *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t data) {
    int spi_device;
    uint8_t out_buf[3] = {0,0,0};;
    uint8_t command_size;

    /* check input variables */
    CHECK_NULL(spi_target);

    spi_device = *(int *)spi_target; /* must check that spi_target is not null beforehand */

    /* prepare frame to be sent */
    if (spi_mux_mode == LGW_SPI_MUX_MODE1) {
        out_buf[0] = spi_mux_target;
        out_buf[1] = WRITE_ACCESS | (address & 0x7F);
        out_buf[2] = data;
        command_size = 3;
    } else {
        out_buf[0] = WRITE_ACCESS | (address & 0x7F);
        out_buf[1] = data;
        command_size = 2;
    }

    spi_ll_select(spi_device);
    spi_ll_bulk_write(spi_device, command_size, out_buf);
    spi_ll_deselect(spi_device);

    SPI_DELAY();
	DEBUG_MSG("Note: SPI write success\n");
	return LGW_SPI_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Simple read */
int lgw_spi_r(void *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t *data) {
    int spi_device;
    uint8_t out_buf[3] = {0,0,0};
    uint8_t command_size;

    /* check input variables */
    CHECK_NULL(spi_target);
    CHECK_NULL(data);

    spi_device = *(int *)spi_target; /* must check that spi_target is not null beforehand */

    /* prepare frame to be sent */
    if (spi_mux_mode == LGW_SPI_MUX_MODE1) {
        out_buf[0] = spi_mux_target;
        out_buf[1] = READ_ACCESS | (address & 0x7F);
        command_size = 3;
    } else {
        out_buf[0] = READ_ACCESS | (address & 0x7F);
        command_size = 2;
    }

    spi_ll_select(spi_device);
    spi_ll_bulk_rw(spi_device, command_size, out_buf);
    spi_ll_deselect(spi_device);

    *data = out_buf[command_size - 1];

    SPI_DELAY();
	DEBUG_MSG("Note: SPI read success\n");
	return LGW_SPI_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Burst (multiple-byte) write */
int lgw_spi_wb(void *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t *data, uint16_t size) {
    int spi_device;
    uint8_t command[2] = {0,0};
    uint8_t command_size;

    /* check input parameters */
    CHECK_NULL(spi_target);

    CHECK_NULL(data);
    if (size == 0) {
        DEBUG_MSG("ERROR: BURST OF NULL LENGTH\n");
        return LGW_SPI_ERROR;
    }

    spi_device = *(int *)spi_target; /* must check that spi_target is not null beforehand */

    /* prepare command byte */
    if (spi_mux_mode == LGW_SPI_MUX_MODE1) {
        command[0] = spi_mux_target;
        command[1] = WRITE_ACCESS | (address & 0x7F);
        command_size = 2;
    } else {
        command[0] = WRITE_ACCESS | (address & 0x7F);
        command_size = 1;
    }

    spi_ll_select(spi_device);
    spi_ll_bulk_write(spi_device, command_size, command);
    spi_ll_bulk_write(spi_device, size, data);
    spi_ll_deselect(spi_device);
    SPI_DELAY();

	DEBUG_MSG("Note: SPI burst write success\n");
	return LGW_SPI_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Burst (multiple-byte) read */
int lgw_spi_rb(void *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t *data, uint16_t size) {
    int spi_device;
    uint8_t command[2] = {0,0};
    uint8_t command_size;

    /* check input parameters */
    CHECK_NULL(spi_target);
    CHECK_NULL(data);
    if (size == 0) {
        DEBUG_MSG("ERROR: BURST OF NULL LENGTH\n");
        return LGW_SPI_ERROR;
    }

    spi_device = *(int *)spi_target; /* must check that spi_target is not null beforehand */

    /* prepare command byte */
    if (spi_mux_mode == LGW_SPI_MUX_MODE1) {
        command[0] = spi_mux_target;
        command[1] = READ_ACCESS | (address & 0x7F);
        command_size = 2;
    } else {
        command[0] = READ_ACCESS | (address & 0x7F);
        command_size = 1;
    }

    spi_ll_select(spi_device);
    spi_ll_bulk_write(spi_device, command_size, command);
    spi_ll_bulk_read(spi_device, size, data);
    spi_ll_deselect(spi_device);

    SPI_DELAY();

	DEBUG_MSG("Note: SPI burst read success\n");
	return LGW_SPI_SUCCESS;
}

/* --- EOF ------------------------------------------------------------------ */

#endif

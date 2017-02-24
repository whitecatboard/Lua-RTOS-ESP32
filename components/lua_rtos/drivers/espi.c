#if 0
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

#define ESPI_USE_IDF 1

#if ESPI_USE_IDF
#include "driver/spi_master.h"
#endif

#include "esp_attr.h"
#include "soc/io_mux_reg.h"
#include "soc/spi_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_reg.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/syslog.h>
#include <sys/driver.h>

#include <drivers/espi.h>
#include <drivers/gpio.h>
#include <drivers/cpu.h>

// Resources used by SPI bus
typedef struct {
	uint8_t setup;
	uint8_t miso;
	uint8_t mosi;
	uint8_t clk;
	uint8_t cs[CPU_LAST_SPI + 1];
#if !ESPI_USE_IDF
	uint32_t speed[CPU_LAST_SPI + 1];
	uint8_t mode[CPU_LAST_SPI + 1];
	uint8_t dirty;
#endif
#if ESPI_USE_IDF
	spi_device_handle_t handle[CPU_LAST_SPI + 1];
#endif
} spi_bus_resources_t;

// Current SPI bus map
static spi_bus_resources_t spi_bus[CPU_LAST_SPI + 1];

// Driver message errors
DRIVER_REGISTER_ERROR(ESPI, espi, InvalidMode, "invalid mode", ESPI_ERR_INVALID_MODE);
DRIVER_REGISTER_ERROR(ESPI, espi, InvalidUnit, "invalid unit", ESPI_ERR_INVALID_UNIT);
DRIVER_REGISTER_ERROR(ESPI, espi, SlaveNotAllowed, "slave mode not allowed", ESPI_ERR_SLAVE_NOT_ALLOWED);
DRIVER_REGISTER_ERROR(ESPI, espi, NotEnoughtMemory, "not enough memory", ESPI_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_ERROR(ESPI, espi, NoMoreDevicesAllowed, "no more devices allowed", ESPI_ERR_NO_MORE_DEVICES_ALLOWED);
DRIVER_REGISTER_ERROR(ESPI, espi, PinNowAllowed, "pin not allowed", ESPI_ERR_PIN_NOT_ALLOWED);
DRIVER_REGISTER_ERROR(ESPI, espi, CannotUpdatePinMap, "cannot update pin map, bus is yet setup", ESPI_ERR_CANNOT_UPDATE_PIN_MAP);

/*
 * Helper functions
 */
static void _espi_init() {
	memset(spi_bus, 0, sizeof(spi_bus_resources_t) * (CPU_LAST_SPI + 1));

	spi_bus[2].miso = 14;
	spi_bus[2].mosi = 12;
	spi_bus[2].clk  = 27;
}

/*
 * Extracted from arduino-esp32 (cores/esp32/esp32-hal-spi.c) for get the clock divisor
 * needed for setup the SIP bus at a desired baud rate
 *
 */
#define FUNC_SPI 1

#define ClkRegToFreq(reg) (CPU_CLK_FREQ / (((reg)->regPre + 1) * ((reg)->regN + 1)))

typedef union {
    uint32_t regValue;
    struct {
        unsigned regL :6;
        unsigned regH :6;
        unsigned regN :6;
        unsigned regPre :13;
        unsigned regEQU :1;
    };
} spiClk_t;

static uint32_t spiFrequencyToClockDiv(uint32_t freq) {
    if(freq >= CPU_CLK_FREQ) {
        return SPI_CLK_EQU_SYSCLK;
    }

    const spiClk_t minFreqReg = { 0x7FFFF000 };
    uint32_t minFreq = ClkRegToFreq((spiClk_t*) &minFreqReg);
    if(freq < minFreq) {
        return minFreqReg.regValue;
    }

    uint8_t calN = 1;
    spiClk_t bestReg = { 0 };
    int32_t bestFreq = 0;

    while(calN <= 0x3F) {
        spiClk_t reg = { 0 };
        int32_t calFreq;
        int32_t calPre;
        int8_t calPreVari = -2;

        reg.regN = calN;

        while(calPreVari++ <= 1) {
            calPre = (((CPU_CLK_FREQ / (reg.regN + 1)) / freq) - 1) + calPreVari;
            if(calPre > 0x1FFF) {
                reg.regPre = 0x1FFF;
            } else if(calPre <= 0) {
                reg.regPre = 0;
            } else {
                reg.regPre = calPre;
            }
            reg.regL = ((reg.regN + 1) / 2);
            calFreq = ClkRegToFreq(&reg);
            if(calFreq == (int32_t) freq) {
                memcpy(&bestReg, &reg, sizeof(bestReg));
                break;
            } else if(calFreq < (int32_t) freq) {
                if(abs(freq - calFreq) < abs(freq - bestFreq)) {
                    bestFreq = calFreq;
                    memcpy(&bestReg, &reg, sizeof(bestReg));
                }
            }
        }
        if(calFreq == (int32_t) freq) {
            break;
        }
        calN++;
    }
    return bestReg.regValue;
}

/*
 * End of extracted code from arduino-esp32
 */

static int espi_get_device_by_cs(int unit, uint8_t cs) {
	int i;

	for(i=0;i<CPU_LAST_SPI + 1;i++) {
		if (spi_bus[unit].cs[i] == cs) return i;
	}

	return -1;
}

static int espi_get_free_device(int unit) {
	int i;

	for(i=0;i<CPU_LAST_SPI + 1;i++) {
		if (spi_bus[unit].cs[i] == 0) return i;
	}

	return -1;
}

static driver_error_t *spi_lock_bus_resources(int unit) {
    driver_unit_lock_error_t *lock_error = NULL;

    // Lock pins
    if ((lock_error = driver_lock(ESPI_DRIVER, unit, GPIO_DRIVER, spi_bus[unit].miso))) {
    	// Revoked lock on pin
    	return driver_lock_error(ESPI_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(ESPI_DRIVER, unit, GPIO_DRIVER, spi_bus[unit].mosi))) {
    	// Revoked lock on pin
    	return driver_lock_error(ESPI_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(ESPI_DRIVER, unit, GPIO_DRIVER, spi_bus[unit].clk))) {
    	// Revoked lock on pin
    	return driver_lock_error(ESPI_DRIVER, lock_error);
    }

    return NULL;
}

static void IRAM_ATTR espi_master_op(int unit, unsigned int word_size, unsigned int len, unsigned char *out, unsigned char *in) {
	unsigned int bytes = word_size * len; // Number of bytes to write / read
	unsigned int idx = 0;

	/*
	 * SPI data buffers hardware registers are 32-bit size, so we use a
	 * transfer buffer for adapt user buffers to buffers expected by hardware, this
	 * buffer is 16-word size (64 bytes)
	 *
	 */
	uint32_t buffer[16]; // Transfer buffer
	uint32_t wd;         // Current word
	unsigned int wdb; 	 // Current byte into current word

	// This is the number of bits to transfer for current chunk
	unsigned int bits;

	bytes = word_size * len;
	while (bytes) {
		// Populate transfer buffer in chunks of 64 bytes
		idx = 0;
		bits = 0;
		while (bytes && (idx < 16)) {
			wd = 0;
			wdb = 4;
			while (bytes && wdb) {
				wd = (wd >> 8);
				if (out) {
					wd |= *out << 24;
					out++;
				} else {
					wd |= 0xff << 24;
				}
				wdb--;
				bytes--;
				bits += 8;
			}

			while (wdb) {
				wd = (wd >> 8);
				wdb--;
			}

			buffer[idx] = wd;
			idx++;
		}

		// Wait for SPI bus ready
		while (READ_PERI_REG(SPI_CMD_REG(unit))&SPI_USR);

		// Load send buffer
	    SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(unit), SPI_USR_MOSI_DBITLEN, bits - 1, SPI_USR_MOSI_DBITLEN_S);
	    SET_PERI_REG_BITS(SPI_MISO_DLEN_REG(unit), SPI_USR_MISO_DBITLEN, bits - 1, SPI_USR_MISO_DBITLEN_S);

	    idx = 0;
	    while ((idx << 5) < bits) {
		    WRITE_PERI_REG((SPI_W0_REG(unit) + (idx << 2)), buffer[idx]);
		    idx++;
	    }

	    // Start transfer
	    SET_PERI_REG_MASK(SPI_CMD_REG(unit), SPI_USR);

	    // Wait for SPI bus ready
		while (READ_PERI_REG(SPI_CMD_REG(unit))&SPI_USR);

		if (in) {
			// Read data into buffer
			idx = 0;
			while ((idx << 5) < bits) {
				buffer[idx] = READ_PERI_REG((SPI_W0_REG(unit) + (idx << 2)));
				idx++;
			}

			memcpy((void *)in, (void *)buffer, bits >> 3);
			in += (bits >> 3);
		}
	}
}

/*
 * Operation functions
 */
driver_error_t *espi_update_pin_map(uint8_t unit, uint8_t miso, uint8_t mosi, uint8_t clk) {
    // Sanity checks
    if ((unit < 1) || (unit > 2)) {
		return driver_operation_error(ESPI_DRIVER, ESPI_ERR_INVALID_UNIT, NULL);
    }

	if (!(GPIO_ALL_IN & (GPIO_BIT_MASK << miso))) {
		return driver_operation_error(ESPI_DRIVER, ESPI_ERR_PIN_NOT_ALLOWED, "miso");
    }

	if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << mosi))) {
		return driver_operation_error(ESPI_DRIVER, ESPI_ERR_PIN_NOT_ALLOWED, "mosi");
    }

	if (!(GPIO_ALL_IN & (GPIO_BIT_MASK << clk))) {
		return driver_operation_error(ESPI_DRIVER, ESPI_ERR_PIN_NOT_ALLOWED, "clk");
    }

    if (spi_bus[unit].setup) {
		return driver_operation_error(ESPI_DRIVER, ESPI_ERR_CANNOT_UPDATE_PIN_MAP, NULL);
    }

    spi_bus[unit].miso = miso;
    spi_bus[unit].mosi = mosi;
    spi_bus[unit].clk  = clk;

    return NULL;
}

driver_error_t *espi_setup(uint8_t unit, uint8_t master, uint8_t cs, uint8_t mode, uint32_t speed, int *deviceid) {
	driver_error_t *error = NULL;

    // Sanity checks
    if ((unit < 1) || (unit > 2)) {
		return driver_operation_error(ESPI_DRIVER, ESPI_ERR_INVALID_UNIT, NULL);
    }

    if (master != 1) {
		return driver_operation_error(ESPI_DRIVER, ESPI_ERR_SLAVE_NOT_ALLOWED, NULL);
    }

    if (mode >= 3) {
		return driver_operation_error(ESPI_DRIVER, ESPI_ERR_INVALID_MODE, NULL);
    }

    // Init bus, if needed
    if (!spi_bus[unit].setup) {
    	// Lock resources need by bus
        if ((error = spi_lock_bus_resources(unit))) {
    		return error;
    	}

		#if ESPI_USE_IDF
        spi_bus_config_t buscfg = {
            .miso_io_num = spi_bus[unit].miso,
            .mosi_io_num = spi_bus[unit].mosi,
            .sclk_io_num = spi_bus[unit].clk,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1
        };

        spi_bus_initialize(unit, &buscfg, 1);
		#else
        printf("setup bus\r\n");

    	printf("miso %d\r\n", spi_bus[unit].miso);
    	printf("mosi %d\r\n", spi_bus[unit].mosi);
    	printf("sck %d\r\n", spi_bus[unit].clk);

    	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[spi_bus[unit].miso], FUNC_SPI);
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[spi_bus[unit].mosi], FUNC_SPI);
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[spi_bus[unit].clk ], FUNC_SPI);

		#endif

        spi_bus[unit].setup = 1;
    }

    // Sanity checks on cs
	if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << cs))) {
		return driver_operation_error(ESPI_DRIVER, ESPI_ERR_PIN_NOT_ALLOWED, "cs");
    }

    // Lock cs pin
    driver_unit_lock_error_t *lock_error = NULL;
    if ((lock_error = driver_lock(ESPI_DRIVER, unit, GPIO_DRIVER, cs))) {
    	// Revoked lock on pin
    	return driver_lock_error(ESPI_DRIVER, lock_error);
    }

    // If there is a device attached to the same cs, remove device from bus prior
    // to add again
    int device = espi_get_device_by_cs(unit, cs);
    if (device >= 0) {
		#if ESPI_USE_IDF
    	spi_bus_remove_device(spi_bus[unit].handle[device]);
		#endif
    } else {
    	device = espi_get_free_device(unit);
    	if (device < 0) {
    		return driver_operation_error(ESPI_DRIVER, ESPI_ERR_NO_MORE_DEVICES_ALLOWED, NULL);
    	}
    }

	#if ESPI_USE_IDF
    // Add device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = speed,
        .mode = mode,
        .spics_io_num = cs,
        .queue_size = 7,
        .pre_cb = NULL
    };

    spi_device_handle_t handle;
	spi_bus_add_device(unit, &devcfg, &handle);
	#else
    printf("setup device %d dev cs %d\r\n", device, cs);
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[cs], FUNC_SPI);
    gpio_pin_output(cs);
    gpio_pin_set(cs);
	#endif

	spi_bus[unit].cs[device] = cs;

	#if ESPI_USE_IDF
	spi_bus[unit].handle[device] = handle;
	#else
	spi_bus[unit].speed[device] = speed;
	spi_bus[unit].mode[device] = mode;
	spi_bus[unit].dirty = 1;
	#endif

	*deviceid = (unit << 8) | device;

	return NULL;
}

driver_error_t *espi_select(int deviceid) {
#if !ESPI_USE_IDF
	int unit = (deviceid & 0xff00) >> 8;
	int device = (deviceid & 0x00ff);

    if (spi_bus[unit].dirty) {
    	printf("dirty\r\n");
    	printf("unit %d\r\n", unit);
    	printf("device %d\r\n", device);
       	printf("mode %d\r\n", spi_bus[unit].mode[device]);
       	printf("speed %d\r\n", spi_bus[unit].speed[device]);
       	printf("cs %d\r\n",spi_bus[unit].cs[device]);

            // Complete operations, if pending
        CLEAR_PERI_REG_MASK(SPI_SLAVE_REG(unit), SPI_TRANS_DONE << 5);
        SET_PERI_REG_MASK(SPI_USER_REG(unit), SPI_CS_SETUP);

        // Set mode
    	switch (spi_bus[unit].mode[device]) {
    		case 0:
    		    // Set CKP to 0
    		    CLEAR_PERI_REG_MASK(SPI_PIN_REG(unit),  SPI_CK_IDLE_EDGE);

    		    // Set CPHA to 0
    		    CLEAR_PERI_REG_MASK(SPI_USER_REG(unit), SPI_CK_OUT_EDGE);

    		    break;

    		case 1:
    		    // Set CKP to 0
    		    CLEAR_PERI_REG_MASK(SPI_PIN_REG(unit),  SPI_CK_IDLE_EDGE);

    		    // Set CPHA to 1
    		    SET_PERI_REG_MASK(SPI_USER_REG(unit), SPI_CK_OUT_EDGE);

    		    break;

    		case 2:
    		    // Set CKP to 1
    		    SET_PERI_REG_MASK(SPI_PIN_REG(unit),  SPI_CK_IDLE_EDGE);

    		    // Set CPHA to 0
    		    CLEAR_PERI_REG_MASK(SPI_USER_REG(unit), SPI_CK_OUT_EDGE);

    		    break;

    		case 3:
    		    // Set CKP to 1
    		    SET_PERI_REG_MASK(SPI_PIN_REG(unit),  SPI_CK_IDLE_EDGE);

    		    // Set CPHA to 1
    		    SET_PERI_REG_MASK(SPI_USER_REG(unit), SPI_CK_OUT_EDGE);
    	}

    	// Set bit order to MSB
        CLEAR_PERI_REG_MASK(SPI_CTRL_REG(unit), SPI_WR_BIT_ORDER | SPI_RD_BIT_ORDER);

        // Enable full-duplex communication
        SET_PERI_REG_MASK(SPI_USER_REG(unit), SPI_DOUTDIN);

        // Configure as master
        WRITE_PERI_REG(SPI_USER1_REG(unit), 0);
    	SET_PERI_REG_BITS(SPI_CTRL2_REG(unit), SPI_MISO_DELAY_MODE, 0, SPI_MISO_DELAY_MODE_S);
    	CLEAR_PERI_REG_MASK(SPI_SLAVE_REG(unit), SPI_SLAVE_MODE);

        // Set clock
        WRITE_PERI_REG(SPI_CLOCK_REG(unit), spiFrequencyToClockDiv(spi_bus[unit].speed[device]));

        // Enable MOSI / MISO / CS
        SET_PERI_REG_MASK(SPI_USER_REG(unit), SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_MOSI | SPI_USR_MISO);
        SET_PERI_REG_MASK(SPI_CTRL2_REG(unit), ((0x4 & SPI_MISO_DELAY_NUM) << SPI_MISO_DELAY_NUM_S));

        CLEAR_PERI_REG_MASK(SPI_USER_REG(unit), SPI_USR_COMMAND);
        SET_PERI_REG_BITS(SPI_USER2_REG(unit), SPI_USR_COMMAND_BITLEN, 0, SPI_USR_COMMAND_BITLEN_S);
        CLEAR_PERI_REG_MASK(SPI_USER_REG(unit), SPI_USR_ADDR);
        SET_PERI_REG_BITS(SPI_USER1_REG(unit), SPI_USR_ADDR_BITLEN, 0, SPI_USR_ADDR_BITLEN_S);

        spi_bus[unit].dirty = 0;
    }

	// Select device
    //if (dev->cs) {
    gpio_pin_clr(spi_bus[unit].cs[device]);
    //}
#endif
    return NULL;
}

driver_error_t *espi_deselect(int deviceid) {
#if !ESPI_USE_IDF
	int unit = (deviceid & 0xff00) >> 8;
	int device = (deviceid & 0x00ff);

	gpio_pin_set(spi_bus[unit].cs[device]);
#endif
    return NULL;
}

driver_error_t * IRAM_ATTR espi_transfer(int deviceid, uint8_t in, uint8_t *out) {
	int unit = (deviceid & 0xff00) >> 8;
	int device = (deviceid & 0x00ff);

	printf("espi_transfer unit %d dev %d\r\n", unit, device);
	#if ESPI_USE_IDF
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=8;
    t.tx_buffer = &in;
    t.rx_buffer = out;

    spi_device_transmit(spi_bus[unit].handle[device], &t);
	#else
    espi_select(deviceid);
    espi_master_op(unit, 1, 1, (unsigned char *)(&in), out);
    espi_deselect(deviceid);
	#endif

    return NULL;
}

DRIVER_REGISTER(ESPI,espi,NULL,_espi_init,NULL);
#endif

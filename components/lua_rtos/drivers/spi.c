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

/*
 * This driver is inspired and takes code from the following projects:
 *
 * arduino-esp32 (https://github.com/espressif/arduino-esp32)
 * esp32-nesemu (https://github.com/espressif/esp32-nesemu
 * esp-open-rtos (https://github.com/SuperHouse/esp-open-rtos)
 *
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <drivers/cpu.h>

// Driver message errors
DRIVER_REGISTER_ERROR(SPI, spi, InvalidMode, "invalid mode", SPI_ERR_INVALID_MODE);
DRIVER_REGISTER_ERROR(SPI, spi, InvalidUnit, "invalid unit", SPI_ERR_INVALID_UNIT);
DRIVER_REGISTER_ERROR(SPI, spi, SlaveNotAllowed, "slave mode not allowed", SPI_ERR_SLAVE_NOT_ALLOWED);
DRIVER_REGISTER_ERROR(SPI, spi, NotEnoughtMemory, "not enough memory", SPI_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_ERROR(SPI, spi, PinNowAllowed, "pin not allowed", SPI_ERR_PIN_NOT_ALLOWED);

// SPI structures
struct spi {
	uint8_t  cs;	  // cs pin for device (if 0 use default cs pin)
    uint32_t speed;   // spi device speed
    uint32_t divisor; // clock divisor
    uint8_t  mode;    // device spi mode
    uint8_t  dirty;   // if 1 device must be reconfigured at next spi_select
    uint8_t  setup;   // device setup?
};

struct spi spi[CPU_LAST_SPI + 1];

/*
 * Helper functions
 */
static void _spi_init() {
	memset(spi, 0, sizeof(struct spi) * (CPU_LAST_SPI + 1));
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

static void spi_pins(int unit, unsigned char *sdi, unsigned char *sdo, unsigned char *sck, unsigned char* cs) {
    switch (unit) {
    	case 1:
        	*sdi = GPIO7;
            *sdo = GPIO8;
            *sck = GPIO6;
            *cs =  GPIO11;
            break;

        case 2:
            *sdi = GPIO12;
            *sdo = GPIO13;
            *sck = GPIO14;
            *cs =  GPIO15;
            break;

        case 3:
            *sdi = GPIO19;
            *sdo = GPIO23;
            *sck = GPIO18;
            *cs =  GPIO5;
            break;
    }
}

// Lock resources needed by the SPI
static driver_error_t *spi_lock_resources(int unit, void *resources) {
	spi_resources_t tmp_spi_resources;

	if (!resources) {
		resources = &tmp_spi_resources;
	}

	spi_resources_t *spi_resources = (spi_resources_t *)resources;
    driver_unit_lock_error_t *lock_error = NULL;

    // Get needed pins
	spi_pins(unit, &spi_resources->sdi, &spi_resources->sdo, &spi_resources->sck, &spi_resources->cs);

    // Lock this pins
    if ((lock_error = driver_lock(SPI_DRIVER, unit, GPIO_DRIVER, spi_resources->sdi))) {
    	// Revoked lock on pin
    	return driver_lock_error(SPI_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(SPI_DRIVER, unit, GPIO_DRIVER, spi_resources->sdo))) {
    	// Revoked lock on pin
    	return driver_lock_error(SPI_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(SPI_DRIVER, unit, GPIO_DRIVER, spi_resources->sck))) {
    	// Revoked lock on pin
    	return driver_lock_error(SPI_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(SPI_DRIVER, unit, GPIO_DRIVER, spi_resources->cs))) {
    	// Revoked lock on pin
    	return driver_lock_error(SPI_DRIVER, lock_error);
    }

    return NULL;
}

/*
 * Operation functions
 *
 */

/*
 * Init pins for a device, and return used pins
 */
void spi_pin_config(int unit, unsigned char sdi, unsigned char sdo, unsigned char sck, unsigned char cs) {
    // Configure pins
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[sdi], FUNC_SPI);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[sdo], FUNC_SPI);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[sck], FUNC_SPI);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[cs ], FUNC_SPI);

    // Set the cs pin to the default cs pin for device
	spi_set_cspin(unit, cs);
}

void IRAM_ATTR spi_master_op(int unit, unsigned int word_size, unsigned int len, unsigned char *out, unsigned char *in) {
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
 * Set the SPI mode for a device. Nothing is changed at hardware level.
 *
 */
driver_error_t *spi_set_mode(int unit, int mode) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	if ((mode < 0) || (mode > 3)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_MODE, NULL);
	}

	struct spi *dev = &spi[unit];

    dev->mode = mode;
    dev->dirty = 1;

    return NULL;
}

/*
 * Set the SPI bit rate for a device and stores the clock divisor needed
 * for this bit rate. Nothing is changed at hardware level.
 */
driver_error_t *spi_set_speed(int unit, unsigned int sck) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	struct spi *dev = &spi[unit];

    dev->speed = sck;
    dev->divisor = spiFrequencyToClockDiv(sck * 1000);
    dev->dirty = 1;

    return NULL;
}

/*
 * Select the device. Prior this we reconfigure the SPI bus
 * to the required settings.
 */
driver_error_t *spi_select(int unit) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	struct spi *dev = &spi[unit];

    if (dev->dirty) {
        // Complete operations, if pending
        CLEAR_PERI_REG_MASK(SPI_SLAVE_REG(unit), SPI_TRANS_DONE << 5);
        SET_PERI_REG_MASK(SPI_USER_REG(unit), SPI_CS_SETUP);

        // Set mode
    	switch (dev->mode) {
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
        WRITE_PERI_REG(SPI_CLOCK_REG(unit), dev->divisor);

        // Enable MOSI / MISO / CS
        SET_PERI_REG_MASK(SPI_USER_REG(unit), SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_MOSI | SPI_USR_MISO);
        SET_PERI_REG_MASK(SPI_CTRL2_REG(unit), ((0x4 & SPI_MISO_DELAY_NUM) << SPI_MISO_DELAY_NUM_S));

        CLEAR_PERI_REG_MASK(SPI_USER_REG(unit), SPI_USR_COMMAND);
        SET_PERI_REG_BITS(SPI_USER2_REG(unit), SPI_USR_COMMAND_BITLEN, 0, SPI_USR_COMMAND_BITLEN_S);
        CLEAR_PERI_REG_MASK(SPI_USER_REG(unit), SPI_USR_ADDR);
        SET_PERI_REG_BITS(SPI_USER1_REG(unit), SPI_USR_ADDR_BITLEN, 0, SPI_USR_ADDR_BITLEN_S);

        dev->dirty = 0;
    }

	// Select device
    if (dev->cs) {
       gpio_pin_clr(dev->cs);
    }

    return NULL;
}

/*
 * Deselect the device
 */
driver_error_t *spi_deselect(int unit) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	struct spi *dev = &spi[unit];

    if (dev->cs) {
        gpio_pin_set(dev->cs);
    }

    return NULL;
}

/*
 * Set the CS pin for the device. This function stores CS pin in
 * device data, and setup CS pin as output and set to the high state
 * (device is not select)
 */
driver_error_t *spi_set_cspin(int unit, int pin) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	struct spi *dev = &spi[unit];

    if (pin != dev->cs) {
        dev->cs = pin;

        if (pin) {
            gpio_pin_output(dev->cs);
            gpio_pin_set(dev->cs);

            dev->dirty = 1;
        }
    }

    return NULL;
}

/*
 * Transfer one word of data, and return the read word of data.
 * The actual number of bits sent depends on the mode of the transfer.
 * This is blocking, and waits for the transfer to complete
 * before returning.  Times out after a certain period.
 */
driver_error_t * IRAM_ATTR spi_transfer(int unit, unsigned int data, unsigned char *read) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

    spi_master_op(unit, 1, 1, (unsigned char *)(&data), read);

    return NULL;
}

/*
 * Send a chunk of 8-bit data.
 */
driver_error_t *spi_bulk_write(int unit, unsigned int nbytes, unsigned char *data) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	taskDISABLE_INTERRUPTS();
    spi_master_op(unit, 1, nbytes, data, NULL);
    taskENABLE_INTERRUPTS();

    return NULL;
}

/*
 * Receive a chunk of 8-bit data.
 */
driver_error_t *spi_bulk_read(int unit, unsigned int nbytes, unsigned char *data) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	taskDISABLE_INTERRUPTS();
    spi_master_op(unit, 1, nbytes, NULL, data);
    taskENABLE_INTERRUPTS();

    return NULL;
}

/*
 * Send and receive a chunk of 8-bit data.
 */
driver_error_t *spi_bulk_rw(int unit, unsigned int nbytes, unsigned char *data) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	unsigned char *read = (unsigned char *)malloc(nbytes);
	if (read) {
	    taskDISABLE_INTERRUPTS();
	    spi_master_op(unit, 1, nbytes, data, read);
	    taskENABLE_INTERRUPTS();

	    memcpy(data, read, nbytes);
		free(read);
	} else {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	return NULL;
}

/*
 * Send a chunk of 16-bit data.
 */
driver_error_t *spi_bulk_write16(int unit, unsigned int words, short *data) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	taskDISABLE_INTERRUPTS();
    spi_master_op(unit, 2, words, (unsigned char *)data, NULL);
    taskENABLE_INTERRUPTS();

    return NULL;
}

/*
 * Receive a chunk of 16-bit data.
 */
driver_error_t *spi_bulk_read16(int unit, unsigned int nbytes, short *data) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	taskDISABLE_INTERRUPTS();
    spi_master_op(unit, 2, nbytes, NULL, (unsigned char *)data);
    taskENABLE_INTERRUPTS();

    return NULL;
}

/*
 * Send a chunk of 32-bit data.
 */
driver_error_t *spi_bulk_write32(int unit, unsigned int words, int *data) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	taskDISABLE_INTERRUPTS();
    spi_master_op(unit, 4, words, (unsigned char *)data, NULL);
    taskENABLE_INTERRUPTS();

    return NULL;
}

driver_error_t *spi_bulk_write32_be(int unit, unsigned int words, int *data) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	int i = 0;

    taskDISABLE_INTERRUPTS();

    if (GET_PERI_REG_MASK(SPI_CTRL_REG(unit), (SPI_WR_BIT_ORDER | SPI_RD_BIT_ORDER))) {
        for(;i < words;i++) {
        	data[i] = __builtin_bswap32(data[i]);
        }
    }

    spi_master_op(unit, 4, words, (unsigned char *)data, NULL);

    taskENABLE_INTERRUPTS();

    return NULL;
}

// Read a huge chunk of data as fast and as efficiently as
// possible.  Switches in to 32-bit mode regardless, and uses
// the enhanced buffer mode.
// Data should be a multiple of 32 bits.
driver_error_t *spi_bulk_read32_be(int unit, unsigned int words, int *data) {
	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	int i = 0;

    taskDISABLE_INTERRUPTS();

    spi_master_op(unit, 4, words, NULL, (unsigned char *)data);

    if (GET_PERI_REG_MASK(SPI_CTRL_REG(unit), (SPI_WR_BIT_ORDER | SPI_RD_BIT_ORDER))) {
        for(;i < words;i++) {
        	data[i] = __builtin_bswap32(data[i]);
        }
    }

    taskENABLE_INTERRUPTS();

    return NULL;
}

/*
 * Return the name of the SPI bus for a device.
 */
const char *spi_name(int unit) {
    static const char *name[CPU_LAST_SPI + 1] = { "spi0", "spi1", "spi2", "spi3" };
    return name[unit];
}

/*
 * Return the pin index of the chip select pin for a device.
 */
int spi_cs_gpio(int unit) {
    struct spi *dev = &spi[unit];

    return dev->cs;
}

/*
 * Return the speed in kHz.
 */
unsigned int spi_get_speed(int unit) {
    struct spi *dev = &spi[unit];

    return dev->speed;
}

// Init a spi device
driver_error_t *spi_init(int unit, int master) {
    struct spi *dev = &spi[unit];

	// Sanity checks
	if ((unit > CPU_LAST_SPI) || (unit < CPU_FIRST_SPI)) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_INVALID_UNIT, NULL);
	}

	if (master != 1) {
		return driver_operation_error(SPI_DRIVER, SPI_ERR_SLAVE_NOT_ALLOWED, NULL);
	}

	// If device is setup, nothing to do
	if (dev->setup) return NULL;

    // Lock resources
    driver_error_t *error;
    spi_resources_t resources;

    if ((error = spi_lock_resources(unit, &resources))) {
		return error;
	}

	// There are not errors, continue with init ...

    // Cotinue with init
	spi_pin_config(unit, resources.sdi, resources.sdo, resources.sck, resources.cs);

	syslog(LOG_INFO,
        "spi%u at pins sdi=%s%d/sdo=%s%d/sck=%s%d", unit,
        gpio_portname(resources.sdi), gpio_name(resources.sdi),
        gpio_portname(resources.sdo), gpio_name(resources.sdo),
        gpio_portname(resources.sck), gpio_name(resources.sck)
    );

    spi_set_mode(unit, 0);

    dev->dirty = 1;
    dev->setup = 1;
    
    return NULL;
}

DRIVER_REGISTER(SPI,spi,NULL,_spi_init,spi_lock_resources);

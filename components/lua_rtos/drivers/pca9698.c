/*
 * Lua RTOS, PCA9698 driver
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#include "sdkconfig.h"

#include <stdint.h>

#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/i2c.h>
#include <drivers/pca9698.h>

// Driver message errors
DRIVER_REGISTER_ERROR(PCA9698, pca9698, InvalidUnit, "invalid unit", PCA9698_ERR_INVALID_UNIT);

uint8_t setup = 0;

/*
 * Helper functions
 */


/*
 * Operation functions
 */

driver_error_t *pca9698_setup() {
	driver_error_t *error;

	if ((error = i2c_setup(CONFIG_PCA9698_I2C, I2C_MASTER, CONFIG_PCA9698_I2C_SPEED, 0, 0))) {
		return error;
	}

	if (!setup) {
		syslog(
				LOG_INFO,
				"GPIO PCA9698 i2c%d, address %x",
				CONFIG_PCA9698_I2C, CONFIG_PCA9698_I2C_ADDRESS
		);
	}

	setup = 1;

	return NULL;
}

void pca_9698_pin_output(uint8_t pin) {
}

void pca_9698_pin_input(uint8_t pin) {
}

void pca_9698_pin_set(uint8_t pin) {
}

void pca_9698_pin_clr(uint8_t pin) {
}

void pca_9698_pin_inv(uint8_t pin) {
}

DRIVER_REGISTER(PCA9698,pca9698,NULL,NULL,NULL);

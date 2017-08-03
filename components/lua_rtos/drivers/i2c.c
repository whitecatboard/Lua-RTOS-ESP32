/*
 * Lua RTOS, I2C driver
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

#include "luartos.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "driver/periph_ctrl.h"

#include <stdint.h>
#include <string.h>

#include <sys/macros.h>
#include <sys/list.h>
#include <sys/mutex.h>
#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/cpu.h>
#include <drivers/i2c.h>

#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL        0x0     /*!< I2C ack value */
#define NACK_VAL       0x1     /*!< I2C nack value */

// Driver locks
driver_unit_lock_t i2c_locks[CPU_LAST_I2C + 1];

// Driver message errors
DRIVER_REGISTER_ERROR(I2C, i2c, CannotSetup, "can't setup", I2C_ERR_CANT_INIT);
DRIVER_REGISTER_ERROR(I2C, i2c, NotSetup, "is not setup", I2C_ERR_IS_NOT_SETUP);
DRIVER_REGISTER_ERROR(I2C, i2c, InvalidUnit, "invalid unit", I2C_ERR_INVALID_UNIT);
DRIVER_REGISTER_ERROR(I2C, i2c, InvalidOperation,"invalid operation", I2C_ERR_INVALID_OPERATION);
DRIVER_REGISTER_ERROR(I2C, i2c, NotEnoughtMemory, "not enough memory", I2C_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_ERROR(I2C, i2c, InvalidTransaction, "invalid transaction", I2C_ERR_INVALID_TRANSACTION);
DRIVER_REGISTER_ERROR(I2C, i2c, AckNotReceived, "not ack received", I2C_ERR_NOT_ACK);
DRIVER_REGISTER_ERROR(I2C, i2c, Timeout, "timeout", I2C_ERR_TIMEOUT);
DRIVER_REGISTER_ERROR(I2C, i2c, PinNowAllowed, "pin not allowed", I2C_ERR_PIN_NOT_ALLOWED);
DRIVER_REGISTER_ERROR(I2C, i2c, CannotChangePinMap, "cannot change pin map once the I2C unit has an attached device", I2C_ERR_CANNOT_CHANGE_PINMAP);

// i2c info needed by driver
i2c_t i2c[CPU_LAST_I2C + 1];

// Transaction list
static struct list transactions;

/*
 * Helper functions
 */
static void i2c_init() {
	int i;

	// Disable i2c modules
	periph_module_disable(PERIPH_I2C0_MODULE);
	periph_module_disable(PERIPH_I2C1_MODULE);

	// Set driver structure to 0;
	memset(i2c, 0, sizeof(i2c_t) * (CPU_LAST_I2C + 1));

	// Init transaction list
    list_init(&transactions, 0);

    // Init mutexes and pin maps
    for(i=0;i < CPU_LAST_I2C + 1;i++) {
        mtx_init(&i2c[i].mtx, NULL, NULL, 0);

        switch (i) {
        case 0:
        	i2c[i].scl = CONFIG_LUA_RTOS_I2C0_SCL;
        	i2c[i].sda = CONFIG_LUA_RTOS_I2C0_SDA;
        	break;

        case 1:
        	i2c[i].scl = CONFIG_LUA_RTOS_I2C1_SCL;
        	i2c[i].sda = CONFIG_LUA_RTOS_I2C1_SDA;
        	break;
        }
    }
}

static driver_error_t *i2c_lock_resources(int unit) {
	driver_unit_lock_error_t *lock_error = NULL;

	// Lock sda
	if ((lock_error = driver_lock(I2C_DRIVER, unit, GPIO_DRIVER, i2c[unit].sda, DRIVER_ALL_FLAGS, "SDA"))) {
    	return driver_lock_error(I2C_DRIVER, lock_error);
    }

	// Lock scl
	if ((lock_error = driver_lock(I2C_DRIVER, unit, GPIO_DRIVER, i2c[unit].scl, DRIVER_ALL_FLAGS, "SCL"))) {
    	return driver_lock_error(I2C_DRIVER, lock_error);
    }

	return NULL;
}

static driver_error_t *i2c_check(int unit) {
    // Sanity checks
	if (!((1 << unit) & CPU_I2C_ALL)) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_UNIT, NULL);
	}

	if (!i2c[unit].setup) {
		return driver_error(I2C_DRIVER, I2C_ERR_IS_NOT_SETUP, NULL);
	}

	return NULL;
}

static driver_error_t *i2c_get_command(int unit, int *transaction, i2c_cmd_handle_t *cmd) {
    if (list_get(&transactions, *transaction, (void **)cmd)) {
    	mtx_unlock(&i2c[unit].mtx);

		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    return NULL;
}

static driver_error_t *i2c_create_or_get_command(int unit, int *transaction, i2c_cmd_handle_t *cmd) {
	driver_error_t *error;

	// If transaction is valid get command, we don't need to create
	if (*transaction != I2C_TRANSACTION_INITIALIZER) {
		if ((error = i2c_get_command(unit, transaction, cmd))) {
			return error;
		}
	} else {
		// Create command
		*cmd = i2c_cmd_link_create();
		if (!*cmd) {
			mtx_unlock(&i2c[unit].mtx);
			return driver_error(I2C_DRIVER, I2C_ERR_NOT_ENOUGH_MEMORY, NULL);
		}

		// Add transaction to list
		if (list_add(&transactions, *cmd, transaction)) {
			*transaction = I2C_TRANSACTION_INITIALIZER;

			i2c_cmd_link_delete(*cmd);
			mtx_unlock(&i2c[unit].mtx);
			return driver_error(I2C_DRIVER, I2C_ERR_NOT_ENOUGH_MEMORY, NULL);
		}
	}

	return NULL;
}

static driver_error_t *i2c_flush_internal(int unit, int *transaction, i2c_cmd_handle_t cmd) {
	// Flush
	esp_err_t err = i2c_master_cmd_begin(unit, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	list_remove(&transactions, *transaction, 0);

    *transaction = I2C_TRANSACTION_INITIALIZER;

	if (err == ESP_FAIL) {
    	mtx_unlock(&i2c[unit].mtx);
		return driver_error(I2C_DRIVER, I2C_ERR_NOT_ACK, NULL);
	} else if (err == ESP_ERR_TIMEOUT) {
    	mtx_unlock(&i2c[unit].mtx);
		return driver_error(I2C_DRIVER, I2C_ERR_TIMEOUT, NULL);
	}

	return NULL;
}

/*
 * Operation functions
 */
driver_error_t *i2c_flush(int unit, int *transaction, int new_transaction) {
	driver_error_t *error;
	i2c_cmd_handle_t cmd = NULL;

	// Get command
	if ((error = i2c_get_command(unit, transaction, &cmd))) {
		return error;
	}

	// Flush
	if ((error = i2c_flush_internal(unit, transaction, cmd))) {
		return error;
	}

	if (new_transaction) {
		// Create a new command
		if ((error = i2c_create_or_get_command(unit, transaction, &cmd))) {
			mtx_unlock(&i2c[unit].mtx);
			return error;
		}
	}

	return NULL;
}

driver_error_t *i2c_pin_map(int unit, int sda, int scl) {
    // Sanity checks
	if (!((1 << unit) & CPU_I2C_ALL)) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_UNIT, NULL);
	}

	mtx_lock(&i2c[unit].mtx);

	if (i2c[unit].setup) {
	   	mtx_unlock(&i2c[unit].mtx);
		return driver_error(I2C_DRIVER, I2C_ERR_CANNOT_CHANGE_PINMAP, NULL);
	}

	// Sanity checks on pinmap
    if ((!(GPIO_ALL_OUT & (GPIO_BIT_MASK << i2c[unit].scl))) && (scl >= 0)) {
    	mtx_unlock(&i2c[unit].mtx);

		return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED, "scl, selected pin cannot be output");
    }

    if ((!(GPIO_ALL_OUT & (GPIO_BIT_MASK << i2c[unit].sda))) && (sda >= 0)) {
    	mtx_unlock(&i2c[unit].mtx);

    	return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED, "sda, selected pin cannot be output");
    }

    if ((!(GPIO_ALL_IN & (GPIO_BIT_MASK << i2c[unit].sda))) && (sda >= 0)) {
    	mtx_unlock(&i2c[unit].mtx);

    	return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED, "sda, selected pin cannot be input");
    }

    if (!TEST_UNIQUE2(i2c[unit].sda, i2c[unit].scl)) {
    	mtx_unlock(&i2c[unit].mtx);
		return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED, "sda and scl must be different");
    }

    // Update pin map, if needed
	if (scl >= 0) {
		i2c[unit].scl = scl;
	}

	if (sda >= 0) {
		i2c[unit].sda = sda;
	}

	mtx_unlock(&i2c[unit].mtx);

	return NULL;
}

driver_error_t *i2c_setup(int unit, int mode, int speed, int addr10_en, int addr) {
	driver_error_t *error;

    // Sanity checks
	if (!((1 << unit) & CPU_I2C_ALL)) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_UNIT, NULL);
	}

	if (speed == -1) {
		speed = 400000;
	}

	mtx_lock(&i2c[unit].mtx);

	// If unit is setup, remove first
	if (i2c[unit].setup) {
		i2c_driver_delete(unit);

		if (unit == 0) {
			periph_module_disable(PERIPH_I2C0_MODULE);
		} else {
			periph_module_disable(PERIPH_I2C1_MODULE);
		}

		i2c[unit].setup = 0;
	}

    if ((error = i2c_lock_resources(unit))) {
    	mtx_unlock(&i2c[unit].mtx);

		return error;
	}

    // Enable module
    if (unit == 0) {
		periph_module_enable(PERIPH_I2C0_MODULE);
	} else {
		periph_module_enable(PERIPH_I2C1_MODULE);
	}

    // Setup
    int buff_len = 0;
    i2c_config_t conf;

    conf.sda_io_num = i2c[unit].sda;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = i2c[unit].scl;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;

    if (mode == I2C_MASTER) {
        conf.mode = I2C_MODE_MASTER;
        conf.master.clk_speed = speed;
        buff_len = 0;
    } else {
    	conf.mode = I2C_MODE_SLAVE;
    	conf.slave.addr_10bit_en = addr10_en;
    	conf.slave.slave_addr = addr;
    	buff_len = 1024;
    }

    i2c_param_config(unit, &conf);
    i2c_driver_install(unit, conf.mode, buff_len, buff_len, 0);

    i2c[unit].mode = mode;
    i2c[unit].setup = 1;

	mtx_unlock(&i2c[unit].mtx);

    syslog(LOG_INFO,
        "i2c%u at pins scl=%s%d/sdc=%s%d at speed %d hz", unit,
        gpio_portname(i2c[unit].scl), gpio_name(i2c[unit].scl),
        gpio_portname(i2c[unit].sda), gpio_name(i2c[unit].sda),
		speed
    );

    return NULL;
}

driver_error_t *i2c_setspeed(int unit, int speed) {
	driver_error_t *error;

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	mtx_lock(&i2c[unit].mtx);

	int half_cycle = (I2C_APB_CLK_FREQ / speed) / 2;

	i2c_set_period(unit, (I2C_APB_CLK_FREQ / speed) - half_cycle - 1,  half_cycle - 1);
	i2c_set_start_timing(unit, half_cycle, half_cycle);
	i2c_set_stop_timing(unit, half_cycle, half_cycle);
	i2c_set_data_timing(unit, half_cycle / 2, half_cycle / 2);

	mtx_unlock(&i2c[unit].mtx);

	return NULL;
}

driver_error_t *i2c_start(int unit, int *transaction) {
	driver_error_t *error;
	i2c_cmd_handle_t cmd = NULL;

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	if (i2c[unit].mode != I2C_MASTER) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION, "only allowed in master mode");
	}

	mtx_lock(&i2c[unit].mtx);

	if ((error = i2c_create_or_get_command(unit, transaction, &cmd))) {
		mtx_unlock(&i2c[unit].mtx);
		return error;
	}

	i2c_master_start(cmd);

	mtx_unlock(&i2c[unit].mtx);

	return NULL;
}

driver_error_t *i2c_stop(int unit, int *transaction) {
	driver_error_t *error;

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	if (i2c[unit].mode != I2C_MASTER) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION, "only allowed in master mode");
	}

	mtx_lock(&i2c[unit].mtx);

	// Get command
	i2c_cmd_handle_t cmd;
    if (list_get(&transactions, *transaction, (void **)&cmd)) {
    	mtx_unlock(&i2c[unit].mtx);

		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

	i2c_master_stop(cmd);

	// Flush
	if ((error = i2c_flush_internal(unit, transaction, cmd))) {
		mtx_unlock(&i2c[unit].mtx);
		return error;
	}

	mtx_unlock(&i2c[unit].mtx);

	return NULL;
}

driver_error_t *i2c_write_address(int unit, int *transaction, char address, int read) {
	driver_error_t *error;

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	if (i2c[unit].mode != I2C_MASTER) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION, NULL);
	}

	mtx_lock(&i2c[unit].mtx);

	// Get command
	i2c_cmd_handle_t cmd;
    if (list_get(&transactions, *transaction, (void **)&cmd)) {
    	mtx_unlock(&i2c[unit].mtx);

		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    i2c_master_write_byte(cmd, address << 1 | (read?I2C_MASTER_READ:I2C_MASTER_WRITE), ACK_CHECK_EN);

    mtx_unlock(&i2c[unit].mtx);

	return NULL;
}

driver_error_t *i2c_write(int unit, int *transaction, char *data, int len) {
	driver_error_t *error;

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	if (i2c[unit].mode != I2C_MASTER) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION, NULL);
	}

	mtx_lock(&i2c[unit].mtx);

	// Get command
	i2c_cmd_handle_t cmd;
    if (list_get(&transactions, *transaction, (void **)&cmd)) {
    	mtx_unlock(&i2c[unit].mtx);

		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    if (len > 1) {
    	i2c_master_write(cmd, (uint8_t *)data, len, ACK_CHECK_EN);
    } else {
        i2c_master_write_byte(cmd, *data, ACK_CHECK_EN);
    }

	mtx_unlock(&i2c[unit].mtx);

    return NULL;
}

driver_error_t *i2c_read(int unit, int *transaction, char *data, int len) {
	driver_error_t *error;

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	if (i2c[unit].mode != I2C_MASTER) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION, NULL);
	}

	mtx_lock(&i2c[unit].mtx);

	// Get command
	i2c_cmd_handle_t cmd;
    if (list_get(&transactions, *transaction, (void **)&cmd)) {
    	mtx_unlock(&i2c[unit].mtx);

		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    if (len > 1) {
    	i2c_master_read(cmd, (uint8_t *)data, len - 1, ACK_VAL);
    }

   	i2c_master_read_byte(cmd, (uint8_t *)(data + len - 1), NACK_VAL);

    mtx_unlock(&i2c[unit].mtx);

    return NULL;
}

DRIVER_REGISTER(I2C,i2c,i2c_locks,i2c_init,NULL);

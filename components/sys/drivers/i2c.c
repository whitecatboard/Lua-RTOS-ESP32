/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, I2C driver
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_I2C

#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "driver/periph_ctrl.h"

#include <stdint.h>
#include <string.h>

#include <sys/macros.h>
#include <sys/list.h>
#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/cpu.h>
#include <drivers/i2c.h>

#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL        0x0     /*!< I2C ack value */
#define NACK_VAL       0x1     /*!< I2C nack value */

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
// Driver locks
driver_unit_lock_t i2c_locks[(CPU_LAST_I2C + 1) * I2C_BUS_DEVICES];
#endif

// Register driver and messages
static void i2c_init();

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
DRIVER_REGISTER_BEGIN(I2C,i2c,i2c_locks,i2c_init,NULL);
#else
DRIVER_REGISTER_BEGIN(I2C,i2c,NULL,i2c_init,NULL);
#endif
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
	DRIVER_REGISTER_ERROR(I2C, i2c, NoMoreDevicesAllowed, "no more devices allowed", I2C_ERR_NO_MORE_DEVICES_ALLOWED);
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
DRIVER_REGISTER_END(I2C,i2c,i2c_locks,i2c_init,NULL);
#else
DRIVER_REGISTER_END(I2C,i2c,NULL,i2c_init,NULL);
#endif

// i2c info needed by driver
i2c_t i2c[CPU_LAST_I2C + 1];

// Transaction list
static struct list transactions;

/*
 * Helper functions
 */

static int i2c_get_free_device(int unit) {
	int i;

	for(i = 0;i < I2C_BUS_DEVICES;i++) {
		if (i2c[unit].device[i].speed == 0) return i;
	}

	return -1;
}

static void i2c_lock(uint8_t unit) {
	xSemaphoreTakeRecursive(i2c[unit].mtx, portMAX_DELAY);
}

static void i2c_unlock(uint8_t unit) {
	xSemaphoreGiveRecursive(i2c[unit].mtx);
}

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
    	i2c[i].mtx = xSemaphoreCreateRecursiveMutex();

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

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
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
#endif

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
    	i2c_unlock(unit);

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
			i2c_unlock(unit);
			return driver_error(I2C_DRIVER, I2C_ERR_NOT_ENOUGH_MEMORY, NULL);
		}

		// Add transaction to list
		if (list_add(&transactions, *cmd, transaction)) {
			*transaction = I2C_TRANSACTION_INITIALIZER;

			i2c_cmd_link_delete(*cmd);
			i2c_unlock(unit);
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
    	i2c_unlock(unit);
		return driver_error(I2C_DRIVER, I2C_ERR_NOT_ACK, NULL);
	} else if (err == ESP_ERR_TIMEOUT) {
    	i2c_unlock(unit);
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
			i2c_unlock(unit);
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

	i2c_lock(unit);

	if (i2c[unit].setup) {
	   	i2c_unlock(unit);
		return driver_error(I2C_DRIVER, I2C_ERR_CANNOT_CHANGE_PINMAP, NULL);
	}

	// Sanity checks on pinmap
    if ((!(GPIO_ALL_OUT & (GPIO_BIT_MASK << i2c[unit].scl))) && (scl >= 0)) {
    	i2c_unlock(unit);

		return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED, "scl, selected pin cannot be output");
    }

    if ((!(GPIO_ALL_OUT & (GPIO_BIT_MASK << i2c[unit].sda))) && (sda >= 0)) {
    	i2c_unlock(unit);

    	return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED, "sda, selected pin cannot be output");
    }

    if ((!(GPIO_ALL_IN & (GPIO_BIT_MASK << i2c[unit].sda))) && (sda >= 0)) {
    	i2c_unlock(unit);

    	return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED, "sda, selected pin cannot be input");
    }

    if (!TEST_UNIQUE2(i2c[unit].sda, i2c[unit].scl)) {
    	i2c_unlock(unit);
		return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED, "sda and scl must be different");
    }

    // Update pin map, if needed
	if (scl >= 0) {
		i2c[unit].scl = scl;
	}

	if (sda >= 0) {
		i2c[unit].sda = sda;
	}

	i2c_unlock(unit);

	return NULL;
}

driver_error_t *i2c_setup(int unit, int mode, int speed, int addr10_en, int addr, int *deviceid) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	driver_error_t *error;
#endif

    // Sanity checks
	if (!((1 << unit) & CPU_I2C_ALL)) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_UNIT, NULL);
	}

	if ((speed < 0) || (speed == 0)) {
		speed = 400000;
	}

	i2c_lock(unit);

	// Setup only once
	if (!i2c[unit].setup) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
		if ((error = i2c_lock_resources(unit))) {
	    	i2c_unlock(unit);

			return error;
		}
#endif

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

	    syslog(LOG_INFO,
	        "i2c%u at pins scl=%s%d/sdc=%s%d", unit,
	        gpio_portname(i2c[unit].scl), gpio_name(i2c[unit].scl),
	        gpio_portname(i2c[unit].sda), gpio_name(i2c[unit].sda)
	    );
	}

	// Get a free device
	int device = i2c_get_free_device(unit);
	if (device < 0) {
		// No more devices
		return driver_error(I2C_DRIVER, I2C_ERR_NO_MORE_DEVICES_ALLOWED, NULL);
	}

	i2c[unit].device[device].speed = speed;

	*deviceid = ((unit << 8) | device);

	i2c_unlock(unit);

    return NULL;
}

driver_error_t *i2c_setspeed(int unit, int speed) {
	driver_error_t *error;

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	i2c_lock(unit);

	int half_cycle = (I2C_APB_CLK_FREQ / speed) / 2;

	i2c_set_period(unit, (I2C_APB_CLK_FREQ / speed) - half_cycle - 1,  half_cycle - 1);
	i2c_set_start_timing(unit, half_cycle, half_cycle);
	i2c_set_stop_timing(unit, half_cycle, half_cycle);
	i2c_set_data_timing(unit, half_cycle / 2, half_cycle / 2);

	i2c_unlock(unit);

	return NULL;
}

driver_error_t *i2c_start(int deviceid, int *transaction) {
	driver_error_t *error;
	i2c_cmd_handle_t cmd = NULL;

	int unit = (deviceid & 0xff00) >> 8;
	int device = (deviceid & 0x00ff);

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	if (i2c[unit].mode != I2C_MASTER) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION, "only allowed in master mode");
	}

	i2c_lock(unit);

	i2c_setspeed(unit, i2c[unit].device[device].speed);

	if ((error = i2c_create_or_get_command(unit, transaction, &cmd))) {
		i2c_unlock(unit);
		return error;
	}

	i2c_master_start(cmd);

	i2c_unlock(unit);

	return NULL;
}

driver_error_t *i2c_stop(int deviceid, int *transaction) {
	driver_error_t *error;

	int unit = (deviceid & 0xff00) >> 8;

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	if (i2c[unit].mode != I2C_MASTER) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION, "only allowed in master mode");
	}

	i2c_lock(unit);

	// Get command
	i2c_cmd_handle_t cmd;
    if (list_get(&transactions, *transaction, (void **)&cmd)) {
    	i2c_unlock(unit);
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

	i2c_master_stop(cmd);

	// Flush
	if ((error = i2c_flush_internal(unit, transaction, cmd))) {
		i2c_unlock(unit);
		return error;
	}

	i2c_unlock(unit);

	return NULL;
}

driver_error_t *i2c_write_address(int deviceid, int *transaction, char address, int read) {
	driver_error_t *error;

	int unit = (deviceid & 0xff00) >> 8;

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	if (i2c[unit].mode != I2C_MASTER) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION, NULL);
	}

	i2c_lock(unit);

	// Get command
	i2c_cmd_handle_t cmd;
    if (list_get(&transactions, *transaction, (void **)&cmd)) {
    	i2c_unlock(unit);

		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    i2c_master_write_byte(cmd, address << 1 | (read?I2C_MASTER_READ:I2C_MASTER_WRITE), ACK_CHECK_EN);

    i2c_unlock(unit);

	return NULL;
}

driver_error_t *i2c_write(int deviceid, int *transaction, char *data, int len) {
	driver_error_t *error;

	int unit = (deviceid & 0xff00) >> 8;

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	if (i2c[unit].mode != I2C_MASTER) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION, NULL);
	}

	i2c_lock(unit);

	// Get command
	i2c_cmd_handle_t cmd;
    if (list_get(&transactions, *transaction, (void **)&cmd)) {
    	i2c_unlock(unit);

		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    if (len > 1) {
    	i2c_master_write(cmd, (uint8_t *)data, len, ACK_CHECK_EN);
    } else {
        i2c_master_write_byte(cmd, *data, ACK_CHECK_EN);
    }

	i2c_unlock(unit);

    return NULL;
}

driver_error_t *i2c_read(int deviceid, int *transaction, char *data, int len) {
	driver_error_t *error;

	int unit = (deviceid & 0xff00) >> 8;

	// Sanity checks
	if ((error = i2c_check(unit))) {
		return error;
	}

	if (i2c[unit].mode != I2C_MASTER) {
		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION, NULL);
	}

	i2c_lock(unit);

	// Get command
	i2c_cmd_handle_t cmd;
    if (list_get(&transactions, *transaction, (void **)&cmd)) {
    	i2c_unlock(unit);

		return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    if (len > 1) {
    	i2c_master_read(cmd, (uint8_t *)data, len - 1, ACK_VAL);
    }

   	i2c_master_read_byte(cmd, (uint8_t *)(data + len - 1), NACK_VAL);

    i2c_unlock(unit);

    return NULL;
}

#endif

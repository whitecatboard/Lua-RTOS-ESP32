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
 * Lua RTOS, I2C driver
 *
 */

#include "luartos.h"

//#if CONFIG_LUA_RTOS_LUA_USE_I2C

#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_private/periph_ctrl.h"

#include "macros.h"
#include "driver.h"
#include "syslog.h"

#include "gpio.h"
#include "cpu.h"
#include "i2c.h"

#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */

// Register driver and messages
static void i2c_init();

DRIVER_REGISTER_BEGIN(I2C,i2c,CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS * ((CPU_LAST_I2C + 1) * I2C_BUS_DEVICES),i2c_init,NULL);
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
DRIVER_REGISTER_END(I2C,i2c,CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS * ((CPU_LAST_I2C + 1) * I2C_BUS_DEVICES),i2c_init,NULL);

// i2c info needed by driver
i2c_t i2c[CPU_LAST_I2C + 1];

/*
 * Helper functions
 */

static int i2c_get_device(int unit, int address) {
    int i;

    for (i = 0; i < I2C_BUS_DEVICES; i++) {
        if ((i2c[unit].device[i].hdnl != NULL) && (i2c[unit].device[i].address == address)) {
            return i;
        }
    }

    return -1;
}

static int i2c_get_free_device(int unit) {
    int i;

    for (i = 0; i < I2C_BUS_DEVICES; i++) {
        if (i2c[unit].device[i].hdnl == NULL)
            return i;
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

    // Set driver structure to 0;
    memset(i2c, 0, sizeof(i2c_t) * (CPU_LAST_I2C + 1));

    // Init mutexes and pin maps
    for (i = 0; i < CPU_LAST_I2C + 1; i++) {
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
    if ((lock_error = driver_lock(I2C_DRIVER, (unit << 8), GPIO_DRIVER, i2c[unit].sda,
    DRIVER_ALL_FLAGS, "SDA"))) {
        return driver_lock_error(I2C_DRIVER, lock_error);
    }

    // Lock scl
    if ((lock_error = driver_lock(I2C_DRIVER, (unit << 8), GPIO_DRIVER, i2c[unit].scl,
    DRIVER_ALL_FLAGS, "SCL"))) {
        return driver_lock_error(I2C_DRIVER, lock_error);
    }

    return NULL;
}

static driver_error_t *i2c_unlock_resources(int unit) {
    // Unlock sda
    driver_unlock(I2C_DRIVER, (unit << 8), GPIO_DRIVER, i2c[unit].sda);

    // Unlock scl
    driver_unlock(I2C_DRIVER, (unit << 8), GPIO_DRIVER, i2c[unit].scl);

    return NULL;
}
#endif

static driver_error_t *i2c_check(int unit) {
    // Sanity checks
    if (!((1 << unit) & CPU_I2C_ALL)) {
        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_UNIT, NULL);
    }

    if (!i2c[unit].hdnl) {
        return driver_error(I2C_DRIVER, I2C_ERR_IS_NOT_SETUP, NULL);
    }

    return NULL;
}

#if 0
static driver_error_t *i2c_get_command(int unit, int *transaction,
        i2c_cmd_handle_t *cmd) {
    if (lstget(&transactions, *transaction, (void **) cmd)) {
        i2c_unlock(unit);

        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    return NULL;
}

static driver_error_t *i2c_create_or_get_command(int unit, int *transaction,
        i2c_cmd_handle_t *cmd) {
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
        if (lstadd(&transactions, *cmd, transaction)) {
            *transaction = I2C_TRANSACTION_INITIALIZER;

            i2c_cmd_link_delete(*cmd);
            i2c_unlock(unit);
            return driver_error(I2C_DRIVER, I2C_ERR_NOT_ENOUGH_MEMORY, NULL);
        }
    }

    return NULL;
}

static driver_error_t *i2c_flush_internal(int unit, int device,
        int *transaction, i2c_cmd_handle_t cmd) {

    esp_err_t err = ESP_OK;

    // Flush
    err = i2c_master_cmd_begin(unit, cmd, 1000 / portTICK_RATE_MS);

    i2c_cmd_link_delete(cmd);
    lstremove(&transactions, *transaction, 0);

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
#endif

/*
 * Operation functions
 */
driver_error_t *i2c_flush(int deviceid, int *transaction, int new_transaction) {
#if 0
    driver_error_t *error;
    i2c_cmd_handle_t cmd = NULL;

    int unit = (deviceid & 0xff00) >> 8;
    int device = (deviceid & 0x00ff);

    // Get command
    if ((error = i2c_get_command(unit, transaction, &cmd))) {
        return error;
    }

    // Flush
    if ((error = i2c_flush_internal(unit, device, transaction, cmd))) {
        return error;
    }

    if (new_transaction) {
        // Create a new command
        if ((error = i2c_create_or_get_command(unit, transaction, &cmd))) {
            i2c_unlock(unit);
            return error;
        }
    }
#endif

    return NULL;
}

driver_error_t *i2c_pin_map(int unit, int sda, int scl) {
    // Sanity checks
    if (!((1 << unit) & CPU_I2C_ALL)) {
        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_UNIT, NULL);
    }

    i2c_lock(unit);

    if (i2c[unit].hdnl) {
        i2c_unlock(unit);
        return driver_error(I2C_DRIVER, I2C_ERR_CANNOT_CHANGE_PINMAP, NULL);
    }

    // Sanity checks on pinmap
    if ((!(GPIO_ALL_OUT & (GPIO_BIT_MASK << scl))) && (scl >= 0)) {
        i2c_unlock(unit);

        return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED,
                "scl, selected pin cannot be output");
    }

    if ((!(GPIO_ALL_OUT & (GPIO_BIT_MASK << sda))) && (sda >= 0)) {
        i2c_unlock(unit);

        return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED,
                "sda, selected pin cannot be output");
    }

    if ((!(GPIO_ALL_IN & (GPIO_BIT_MASK << sda))) && (sda >= 0)) {
        i2c_unlock(unit);

        return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED,
                "sda, selected pin cannot be input");
    }

    if (!TEST_UNIQUE2(sda, scl)) {
        i2c_unlock(unit);
        return driver_error(I2C_DRIVER, I2C_ERR_PIN_NOT_ALLOWED,
                "sda and scl must be different");
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

driver_error_t *i2c_attach(int unit, int mode, int speed, int addr10_en, int addr, int *deviceid) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_error_t *error;
#endif

    esp_err_t err;
    int device = 0;

    // Sanity checks
    if (!((1 << unit) & CPU_I2C_ALL)) {
        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_UNIT, NULL);
    }

    if ((speed < 0) || (speed == 0)) {
        speed = 400000;
    }

    i2c_lock(unit);

    // Setup bus only once
    if (!i2c[unit].hdnl) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
        if ((error = i2c_lock_resources(unit))) {
            i2c_unlock(unit);
            return error;
        }
#endif
        // Configure bus
        i2c_master_bus_config_t i2c_bus_config = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = unit,
            .scl_io_num = i2c[unit].scl,
            .sda_io_num = i2c[unit].sda,
        };

        err = i2c_new_master_bus(&i2c_bus_config, &i2c[unit].hdnl);
        if (err == ESP_ERR_NO_MEM) {
            i2c_unlock(unit);
        	return driver_error(I2C_DRIVER, I2C_ERR_NOT_ENOUGH_MEMORY, NULL);
        } else if (err == ESP_ERR_NOT_FOUND) {
            i2c_unlock(unit);
        	return driver_error(I2C_DRIVER, I2C_ERR_NO_MORE_DEVICES_ALLOWED, NULL);
		}
    }

    // Setup device only once
    if (i2c_get_device(unit, addr) < 0) {
        // Get a free device
        device = i2c_get_free_device(unit);
        if (device < 0) {
            // No more devices
        	i2c_del_master_bus(i2c[unit].hdnl);
        	i2c[unit].hdnl = NULL;
        	
            return driver_error(I2C_DRIVER, I2C_ERR_NO_MORE_DEVICES_ALLOWED, NULL);
        }

        // Configure device
        i2c_device_config_t i2c_dev_conf = {
        	.dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .scl_speed_hz = speed,
            .device_address = addr,
        };

        err = i2c_master_bus_add_device(i2c[unit].hdnl, &i2c_dev_conf, &i2c[unit].device[device].hdnl);
        if (err == ESP_ERR_NO_MEM) {
        	i2c_del_master_bus(i2c[unit].hdnl);
        	i2c[unit].hdnl = NULL;
            i2c_unlock(unit);
        	return driver_error(I2C_DRIVER, I2C_ERR_NOT_ENOUGH_MEMORY, NULL);
        }
    }

    i2c[unit].mode = mode;

    *deviceid = ((unit << 8) | device);

    i2c_unlock(unit);

    syslog(LOG_INFO, "i2c%u at pins scl=%s%d/sda=%s%d", unit,
            gpio_portname(i2c[unit].scl), gpio_name(i2c[unit].scl),
            gpio_portname(i2c[unit].sda), gpio_name(i2c[unit].sda));

    return NULL;
}

driver_error_t *i2c_detach(int deviceid) {
    driver_error_t *error;

    int unit = (deviceid & 0xff00) >> 8;
    int device = (deviceid & 0x00ff);

    // Sanity checks
    if ((error = i2c_check(unit))) {
        return error;
    }

    i2c_lock(unit);

    // Remove device
    i2c_master_bus_rm_device(i2c[unit].device[device].hdnl);
    i2c[unit].device[device].hdnl = NULL;

    // Check if all devices are unused or not
    int i;
    int no_devices = 1;

    for (i = 0; i < I2C_BUS_DEVICES; i++) {
        if (i2c[unit].device[i].hdnl != NULL) {
            no_devices = 0;
            break;
        }
    }

    if (no_devices) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
        // Unlock resources
        i2c_unlock_resources(unit);
#endif

        // Remove bus
        i2c_del_master_bus(i2c[unit].hdnl);
        i2c[unit].hdnl = NULL;
    }

    i2c_unlock(unit);

    return NULL;
}

driver_error_t *i2c_setspeed(int deviceid, int speed) {
#if 0
    driver_error_t *error;

    int unit = (deviceid & 0xff00) >> 8;

    // Sanity checks
    if ((error = i2c_check(unit))) {
        return error;
    }

    i2c_lock(unit);

    if (i2c[unit].speed == speed) {
        i2c_unlock(unit);
        return NULL;
    }

    int half_cycle = (APB_CLK_FREQ / speed) / 2;

    i2c_set_period(unit, (APB_CLK_FREQ / speed) - half_cycle - 1, half_cycle - 1);
    i2c_set_start_timing(unit, half_cycle, half_cycle);
    i2c_set_stop_timing(unit, half_cycle, half_cycle);
    i2c_set_data_timing(unit, half_cycle / 2, half_cycle / 2);

    i2c[unit].speed = speed;

    i2c_unlock(unit);
#endif
    return NULL;
}

driver_error_t *i2c_start(int deviceid, int *transaction) {
#if 0
    driver_error_t *error;
    i2c_cmd_handle_t cmd = NULL;

    int unit = (deviceid & 0xff00) >> 8;
    int device = (deviceid & 0x00ff);

    // Sanity checks
    if ((error = i2c_check(unit))) {
        return error;
    }

    if (i2c[unit].mode != I2C_MASTER) {
        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION,
                "only allowed in master mode");
    }

    i2c_lock(unit);

    if (i2c[unit].speed != i2c[unit].device[device].speed) {
        i2c_setspeed(deviceid, i2c[unit].device[device].speed);
    }

    if ((error = i2c_create_or_get_command(unit, transaction, &cmd))) {
        i2c_unlock(unit);
        return error;
    }

    i2c_master_start(cmd);

    i2c_unlock(unit);
#endif

    return NULL;
}

driver_error_t *i2c_stop(int deviceid, int *transaction) {
#if 0
    driver_error_t *error;

    int unit = (deviceid & 0xff00) >> 8;
    int device = (deviceid & 0x00ff);

    // Sanity checks
    if ((error = i2c_check(unit))) {
        return error;
    }

    if (i2c[unit].mode != I2C_MASTER) {
        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION,
                "only allowed in master mode");
    }

    i2c_lock(unit);

    // Get command
    i2c_cmd_handle_t cmd;
    if (lstget(&transactions, *transaction, (void **) &cmd)) {
        i2c_unlock(unit);
        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    if (i2c[unit].device[device].reading) {
        uint8_t dummy;
        i2c_master_read_byte(cmd, (uint8_t *) (&dummy), I2C_MASTER_NACK);
    }

    i2c_master_stop(cmd);

    // Flush
    if ((error = i2c_flush_internal(unit, device, transaction, cmd))) {
        i2c_unlock(unit);
        return error;
    }

    i2c[unit].device[device].reading = 0;

    i2c_unlock(unit);
#endif

    return NULL;
}

driver_error_t *i2c_write_address(int deviceid, int *transaction, char address,
        int read) {
#if 0
    driver_error_t *error;

    int unit = (deviceid & 0xff00) >> 8;
    int device = (deviceid & 0x00ff);

    // Sanity checks
    if ((error = i2c_check(unit))) {
        return error;
    }

    if (i2c[unit].mode != I2C_MASTER) {
        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION,
                "only allowed in master mode");
    }

    i2c_lock(unit);

    // Get command
    i2c_cmd_handle_t cmd;
    if (lstget(&transactions, *transaction, (void **) &cmd)) {
        i2c_unlock(unit);

        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    i2c[unit].device[device].reading = read;

    i2c_master_write_byte(cmd,
            address << 1 | (read ? I2C_MASTER_READ : I2C_MASTER_WRITE),
            ACK_CHECK_EN);

    i2c_unlock(unit);
#endif

    return NULL;
}
/*
driver_error_t *i2c_write(int deviceid, int *transaction, char *data, int len) {
#if 0
    driver_error_t *error;

    int unit = (deviceid & 0xff00) >> 8;
    int device = (deviceid & 0x00ff);

    // Sanity checks
    if ((error = i2c_check(unit))) {
        return error;
    }

    if (i2c[unit].mode != I2C_MASTER) {
        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION,
                "only allowed in master mode");
    }

    i2c_lock(unit);

    // Get command
    i2c_cmd_handle_t cmd;
    if (lstget(&transactions, *transaction, (void **) &cmd)) {
        i2c_unlock(unit);

        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    if (i2c[unit].device[device].reading) {
        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION,
                "transaction is for read");
    }

    if (len > 1) {
        i2c_master_write(cmd, (uint8_t *) data, len, ACK_CHECK_EN);
    } else {
        i2c_master_write_byte(cmd, *data, ACK_CHECK_EN);
    }

    i2c_unlock(unit);
#endif

    return NULL;
}

driver_error_t *i2c_read(int deviceid, int *transaction, char *data, int len) {
#if 0
    driver_error_t *error;

    int unit = (deviceid & 0xff00) >> 8;
    int device = (deviceid & 0x00ff);

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
    if (lstget(&transactions, *transaction, (void **) &cmd)) {
        i2c_unlock(unit);

        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_TRANSACTION, NULL);
    }

    if (!i2c[unit].device[device].reading) {
        return driver_error(I2C_DRIVER, I2C_ERR_INVALID_OPERATION,
                "transaction is for write");
    }

    if (len > 1) {
        i2c_master_read(cmd, (uint8_t *) data, len, I2C_MASTER_LAST_NACK);
    } else {
        i2c_master_read_byte(cmd, (uint8_t *) data, I2C_MASTER_ACK);
    }

    i2c_unlock(unit);
#endif
    return NULL;
}
*/

bool i2c_probe(int deviceid, uint16_t address) {
    driver_error_t *error;
    int unit = (deviceid & 0xff00) >> 8;

    // Sanity checks
    if ((error = i2c_check(unit))) {
    	free(error);
        return false;
    }

	return ((i2c_master_probe(i2c[unit].hdnl, address, 1000)) == ESP_OK);
}

driver_error_t *i2c_write(int deviceid, uint8_t *data, int len) {
    driver_error_t *error;
    esp_err_t err;

    int unit = (deviceid & 0xff00) >> 8;
    int device = (deviceid & 0x00ff);

    // Sanity checks
    if ((error = i2c_check(unit))) {
        return error;
    }

    i2c_lock(unit);
	err = i2c_master_transmit(i2c[unit].device[device].hdnl, data, len, 1000);
    i2c_unlock(unit);

    if (err == ESP_ERR_TIMEOUT) {
    	return driver_error(I2C_DRIVER, I2C_ERR_TIMEOUT, NULL);
    }

	return NULL;
}

driver_error_t *i2c_multiple_write(int deviceid, uint8_t *d1, int s1, uint8_t *d2, int s2) {
    driver_error_t *error;
    esp_err_t err = ESP_OK;

    int unit = (deviceid & 0xff00) >> 8;
    int device = (deviceid & 0x00ff);

    // Sanity checks
    if ((error = i2c_check(unit))) {
        return error;
    }

	i2c_operation_job_t i2c_ops[] = {
	    { .command = I2C_MASTER_CMD_START },
	    { .command = I2C_MASTER_CMD_WRITE, .write = { .ack_check = false, .data = d1, .total_bytes = s1 } },
	    { .command = I2C_MASTER_CMD_WRITE, .write = { .ack_check = false, .data = d2, .total_bytes = s2 } },
	    { .command = I2C_MASTER_CMD_STOP },
	};

    i2c_lock(unit);
	err = i2c_master_execute_defined_operations(i2c[unit].device[device].hdnl, i2c_ops, sizeof(i2c_ops) / sizeof(i2c_operation_job_t), 1000);
    i2c_unlock(unit);

    if (err == ESP_ERR_TIMEOUT) {
    	return driver_error(I2C_DRIVER, I2C_ERR_TIMEOUT, NULL);
    }

	return NULL;
}

driver_error_t *i2c_read(int deviceid, uint8_t *data, int len) {
    driver_error_t *error;
    esp_err_t err;
    uint8_t min_read_buff[2];

    int unit = (deviceid & 0xff00) >> 8;
    int device = (deviceid & 0x00ff);

    // Sanity checks
    if ((error = i2c_check(unit))) {
        return error;
    }

    i2c_lock(unit);
    if (len == 1) {
    	err = i2c_master_receive(i2c[unit].device[device].hdnl, min_read_buff, sizeof(min_read_buff), 1000);
    	*data = min_read_buff[0];
    } else {
    	err = i2c_master_receive(i2c[unit].device[device].hdnl, data, len, 1000);
    }
    i2c_unlock(unit);

    if (err == ESP_ERR_TIMEOUT) {
    	return driver_error(I2C_DRIVER, I2C_ERR_TIMEOUT, NULL);
    }

	return NULL;
}

driver_error_t *i2c_write_read(int deviceid, uint8_t *dataw, int lenw, uint8_t *datar, int lenr) {
    driver_error_t *error;
    esp_err_t err;
    uint8_t min_read_buff[2];

    int unit = (deviceid & 0xff00) >> 8;
    int device = (deviceid & 0x00ff);

    // Sanity checks
    if ((error = i2c_check(unit))) {
        return error;
    }

    i2c_lock(unit);
    if (lenr == 1) {
    	err = i2c_master_transmit_receive(i2c[unit].device[device].hdnl, dataw, lenw, min_read_buff, sizeof(min_read_buff), 1000);
    	*datar = min_read_buff[0];
    } else {
    	err = i2c_master_transmit_receive(i2c[unit].device[device].hdnl, dataw, lenw, datar, lenr, 1000);
    }
    i2c_unlock(unit);

    if (err == ESP_ERR_TIMEOUT) {
    	return driver_error(I2C_DRIVER, I2C_ERR_TIMEOUT, NULL);
    }

	return NULL;
}

//#endif

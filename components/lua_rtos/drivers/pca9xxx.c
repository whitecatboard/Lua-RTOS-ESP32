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
 * Lua RTOS, pca9xxx driver
 *
 */

#include "luartos.h"

#if EXTERNAL_GPIO

#include "esp_attr.h"

#include <stdint.h>
#include <string.h>

#include <drivers/gpio.h>

#include <sys/status.h>
#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/i2c.h>
#include <drivers/pca9xxx.h>

static pca_9xxx_t *pca_9xxx = NULL;

static driver_error_t * pca9xxx_read_all_register(uint8_t reg, uint8_t *val);

/*
 * Helper functions
 */

static void pca_9xxx_lock() {
	xSemaphoreTakeRecursive(pca_9xxx->mtx, portMAX_DELAY);
}

static void pca_9xxx_unlock() {
	xSemaphoreGiveRecursive(pca_9xxx->mtx);
}

// PCA968 task. This task waits for a direct task notification and read all pins
// for release the PCA968 INT.
static void pca_9xxx_task(void *arg) {
	uint8_t i, j, pin, latch[PCA9xxx_BANKS], current[PCA9xxx_BANKS];

    for(;;) {
    	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        pca_9xxx_lock();

        // Get current latch values
        memcpy(latch, pca_9xxx->latch, sizeof(pca_9xxx->latch));

        // Read all pins and latch
        pca9xxx_read_all_register(0, pca_9xxx->latch);

        memcpy(current, pca_9xxx->latch, sizeof(pca_9xxx->latch));

		pca_9xxx_unlock();

        // Process interrupts
		for(i = 0; i < PCA9xxx_BANKS; i++) {
			pin = (i << 3);
			for(j = 0; j < 8;j++, pin++) {
				if (pca_9xxx->isr_func[pin]) {
					switch (pca_9xxx->isr_type[pin]) {
						case GPIO_INTR_DISABLE:
							break;

						case GPIO_INTR_POSEDGE:
						case GPIO_INTR_HIGH_LEVEL:
							if ((current[i] & (1 << j)) && !(latch[i] & (1 << j))) {
								pca_9xxx->isr_func[pin](pca_9xxx->isr_args[pin]);
							}
							break;

						case GPIO_INTR_NEGEDGE:
						case GPIO_INTR_LOW_LEVEL:
							if (!(current[i] & (1 << j)) && (latch[i] & (1 << j))) {
								pca_9xxx->isr_func[pin](pca_9xxx->isr_args[pin]);
							}
							break;

						case GPIO_INTR_ANYEDGE:
							if (((current[i] & (1 << j))) != (latch[i] & (1 << j))) {
								pca_9xxx->isr_func[pin](pca_9xxx->isr_args[pin]);
							}
							break;
					}
				}
			}
		}
    }
}

// PCA968 ISR
//
// When some pin changes in PCA968 an interrupt is generated and it is not
// released until all pins are read.
//
// We process the interrupt as a deferred interrupt. We simply done a direct
// task notification, and the interrupt will be processed later in a task.
static void IRAM_ATTR pca9xxx_isr(void* arg) {
    portBASE_TYPE high_priority_task_awoken = 0;

    vTaskNotifyGiveFromISR(pca_9xxx->task, &high_priority_task_awoken);
    if (high_priority_task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

// Write to a PCA968 register
static driver_error_t *pca9xxx_write_register(uint8_t reg, uint8_t val) {
	int transaction = I2C_TRANSACTION_INITIALIZER;
	driver_error_t *error;
	uint8_t buff[2];

	buff[0] = reg;
	buff[1] = val;

	error = i2c_start(pca_9xxx->i2cdevice, &transaction);if (error) return error;
	error = i2c_write_address(pca_9xxx->i2cdevice, &transaction, CONFIG_PCA9xxx_I2C_ADDRESS, 0);if (error) return error;
	error = i2c_write(pca_9xxx->i2cdevice, &transaction, (char *)&buff, sizeof(buff));if (error) return error;
	error = i2c_stop(pca_9xxx->i2cdevice, &transaction);if (error) return error;

	return NULL;
}

// Read from a PCA968 register
static driver_error_t * pca9xxx_read_all_register(uint8_t reg, uint8_t *val) {
	int transaction = I2C_TRANSACTION_INITIALIZER;
	driver_error_t *error;
	uint8_t buff[1];

	buff[0] = reg & 0b10000000;

	error = i2c_start(pca_9xxx->i2cdevice, &transaction);if (error) return error;
	error = i2c_write_address(pca_9xxx->i2cdevice, &transaction, CONFIG_PCA9xxx_I2C_ADDRESS, 0);if (error) return error;
	error = i2c_write(pca_9xxx->i2cdevice, &transaction, (char *)&buff, 1);if (error) return error;
	error = i2c_start(pca_9xxx->i2cdevice, &transaction);if (error) return error;
	error = i2c_write_address(pca_9xxx->i2cdevice, &transaction, CONFIG_PCA9xxx_I2C_ADDRESS, 1);if (error) return error;
	error = i2c_read(pca_9xxx->i2cdevice, &transaction, (char *)val, 5);if (error) return error;
	error = i2c_stop(pca_9xxx->i2cdevice, &transaction);if (error) return error;

	return NULL;
}

/*
 * Operation functions
 */

driver_error_t *pca9xxx_setup() {
	driver_error_t *error;
	int i2cdevice;

	if ((error = i2c_setup(CONFIG_PCA9xxx_I2C, I2C_MASTER, CONFIG_PCA9xxx_I2C_SPEED, 0, 0, &i2cdevice))) {
		return error;
	}

	if (!pca_9xxx) {
		// Create pca_9xxx data
		pca_9xxx = (pca_9xxx_t *)calloc(1, sizeof(pca_9xxx_t) * PCA9xxx_BANKS);
		if (!pca_9xxx) {
			return driver_error(GPIO_DRIVER, GPIO_ERR_NOT_ENOUGH_MEMORY, NULL);
		}

		pca_9xxx->i2cdevice = i2cdevice;

		// Init mutex
		pca_9xxx->mtx = xSemaphoreCreateRecursiveMutex();

		syslog(
				LOG_INFO,
				"GPIO EXTENDER %s at i2c%d, address %x",
				EXTERNAL_GPIO_NAME, CONFIG_PCA9xxx_I2C, CONFIG_PCA9xxx_I2C_ADDRESS
		);

		pca_9xxx_lock();

		// Configure all pins as output / logic level 0
		pca9xxx_write_register(0x18, 0x00);
		pca9xxx_write_register(0x19, 0x00);
		pca9xxx_write_register(0x1a, 0x00);
		pca9xxx_write_register(0x1b, 0x00);
		pca9xxx_write_register(0x1c, 0x00);

		pca9xxx_write_register(0x8, 0x00);
		pca9xxx_write_register(0x9, 0x00);
		pca9xxx_write_register(0xa, 0x00);
		pca9xxx_write_register(0xb, 0x00);
		pca9xxx_write_register(0xc, 0x00);

		// Configure interrupts
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
		driver_unit_lock_error_t *lock_error = NULL;

		// Lock resources
		if ((lock_error = driver_lock(GPIO_DRIVER, 0, GPIO_DRIVER, CONFIG_PCA9xxx_INT, 0, NULL))) {
			pca_9xxx_unlock();

			// Revoked lock on pin
			return driver_lock_error(GPIO_DRIVER, lock_error);
		}
#endif

		BaseType_t xReturn = xTaskCreatePinnedToCore(pca_9xxx_task, "pca9xxx", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &pca_9xxx->task, xPortGetCoreID());
		if (xReturn != pdPASS) {
			pca_9xxx_unlock();

			return driver_error(GPIO_DRIVER, GPIO_ERR_NOT_ENOUGH_MEMORY, NULL);
		}

		gpio_pin_input(CONFIG_PCA9xxx_INT);
		gpio_isr_attach(CONFIG_PCA9xxx_INT, pca9xxx_isr, GPIO_INTR_NEGEDGE, NULL);

		// Enable interrupts on all pins
		pca9xxx_write_register(0x20, 0x00);
		pca9xxx_write_register(0x21, 0x00);
		pca9xxx_write_register(0x22, 0x00);
		pca9xxx_write_register(0x23, 0x00);
		pca9xxx_write_register(0x24, 0x00);

		pca_9xxx_unlock();

		// Read all inputs and latch it
		xTaskNotifyGive(pca_9xxx->task);

		syslog(
				LOG_INFO,
				"GPIO EXTENDER %s i2c%d, interrupts enabled on %s%d",
				EXTERNAL_GPIO_NAME,
				CONFIG_PCA9xxx_I2C,
				gpio_portname(CONFIG_PCA9xxx_INT),
				gpio_name(CONFIG_PCA9xxx_INT)
		);
	}

	return NULL;
}

driver_error_t *pca_9xxx_pin_output(uint8_t pin) {
	uint8_t port = PCA9xxx_GPIO_BANK_NUM(pin);
	uint8_t pinmask = (1 << PCA9xxx_GPIO_BANK_POS(pin));

	if (!pca_9xxx) pca9xxx_setup();

	// Update direction. For input set bit to 0.
	pca_9xxx_lock();
	pca_9xxx->direction[port] &= ~pinmask;
	pca_9xxx_unlock();

	driver_error_t *error;

	error = pca9xxx_write_register(0x18 + port, pca_9xxx->direction[port]);

	return error;
}

driver_error_t *pca_9xxx_pin_input(uint8_t pin) {
	uint8_t port = PCA9xxx_GPIO_BANK_NUM(pin);
	uint8_t pinmask = (1 << PCA9xxx_GPIO_BANK_POS(pin));

	if (!pca_9xxx) pca9xxx_setup();

	// Update direction. For input set bit to 1.
	pca_9xxx_lock();
	pca_9xxx->direction[port] |= pinmask;
	pca_9xxx_unlock();

	driver_error_t *error;

	error = pca9xxx_write_register(0x18 + port, pca_9xxx->direction[port]);

	return error;
}

driver_error_t *pca_9xxx_pin_set(uint8_t pin) {
	uint8_t port = PCA9xxx_GPIO_BANK_NUM(pin);
	uint8_t pinmask = (1 << PCA9xxx_GPIO_BANK_POS(pin));

	if (!pca_9xxx) pca9xxx_setup();

	// Update latch.
	pca_9xxx_lock();
	pca_9xxx->latch[port] |= pinmask;
	pca_9xxx_unlock();

	driver_error_t *error;

	error = pca9xxx_write_register(0x08 + port, pca_9xxx->latch[port]);

	return error;
}

driver_error_t *pca_9xxx_pin_clr(uint8_t pin) {
	uint8_t port = PCA9xxx_GPIO_BANK_NUM(pin);
	uint8_t pinmask = (1 << PCA9xxx_GPIO_BANK_POS(pin));

	if (!pca_9xxx) pca9xxx_setup();

	// Update latch.
	pca_9xxx_lock();
	pca_9xxx->latch[port] &= ~pinmask;
	pca_9xxx_unlock();

	driver_error_t *error;

	error = pca9xxx_write_register(0x08 + port, pca_9xxx->latch[port]);

	return error;
}

driver_error_t *pca_9xxx_pin_inv(uint8_t pin) {
	uint8_t port = PCA9xxx_GPIO_BANK_NUM(pin);
	uint8_t pinmask = (1 << PCA9xxx_GPIO_BANK_POS(pin));

	if (!pca_9xxx) pca9xxx_setup();

	// Update latch.
	pca_9xxx_lock();
	pca_9xxx->latch[port] = (pca_9xxx->latch[port] & ~pinmask) | (((!(pca_9xxx->latch[port] & pinmask)) & pinmask));
	pca_9xxx_unlock();

	driver_error_t *error;

	error = pca9xxx_write_register(0x08 + port, pca_9xxx->latch[port]);

	return error;
}

uint8_t pca_9xxx_pin_get(uint8_t pin) {
	uint8_t port = PCA9xxx_GPIO_BANK_NUM(pin);
	uint8_t pinmask = (1 << PCA9xxx_GPIO_BANK_POS(pin));
	uint8_t val;

	if (!pca_9xxx) pca9xxx_setup();

	pca_9xxx_lock();
	val = ((pca_9xxx->latch[port] & pinmask) != 0);
	pca_9xxx_unlock();

	return val;
}

driver_error_t *pca_9xxx_pin_input_mask(uint8_t port, uint8_t pinmask) {
	if (!pca_9xxx) pca9xxx_setup();

	// Update direction. For input set bit to 1.
	pca_9xxx_lock();
	pca_9xxx->direction[port] |= pinmask;
	pca_9xxx_unlock();

	driver_error_t *error;

	error = pca9xxx_write_register(0x18 + port, pca_9xxx->direction[port]);

	return error;
}

driver_error_t *pca_9xxx_pin_output_mask(uint8_t port, uint8_t pinmask) {
	if (!pca_9xxx) pca9xxx_setup();

	// Update direction. For output set bit to 0.
	pca_9xxx_lock();
	pca_9xxx->direction[port] &= ~pinmask;
	pca_9xxx_unlock();

	driver_error_t *error;

	error = pca9xxx_write_register(0x18 + port, pca_9xxx->direction[port]);

	return error;
}

driver_error_t * pca_9xxx_pin_set_mask(uint8_t port, uint8_t pinmask) {
	if (!pca_9xxx) pca9xxx_setup();

	// Update latch.
	pca_9xxx_lock();
	pca_9xxx->latch[port] |= pinmask;
	pca_9xxx_unlock();

	driver_error_t *error;

	error = pca9xxx_write_register(0x08 + port, pca_9xxx->latch[port]);

	return error;
}

driver_error_t *pca_9xxx_pin_clr_mask(uint8_t port, uint8_t pinmask) {
	if (!pca_9xxx) pca9xxx_setup();

	// Update latch.
	pca_9xxx_lock();
	pca_9xxx->latch[port] &= ~pinmask;
	pca_9xxx_unlock();

	driver_error_t *error;

	error = pca9xxx_write_register(0x08 + port, pca_9xxx->latch[port]);

	return error;
}

driver_error_t *pca_9xxx_pin_inv_mask(uint8_t port, uint8_t pinmask) {
	if (!pca_9xxx) pca9xxx_setup();

	// Update latch.
	pca_9xxx_lock();
	pca_9xxx->latch[port] = (pca_9xxx->latch[port] & ~pinmask) | (((!(pca_9xxx->latch[port] & pinmask)) & pinmask));
	pca_9xxx_unlock();

	driver_error_t *error;

	error = pca9xxx_write_register(0x08 + port, pca_9xxx->latch[port]);

	return error;
}

void pca_9xxx_pin_get_mask(uint8_t port, uint8_t pinmask, uint8_t *value) {
	if (!pca_9xxx) pca9xxx_setup();

	pca_9xxx_lock();
	*value = (pca_9xxx->latch[port] & pinmask);
	pca_9xxx_unlock();
}

uint64_t IRAM_ATTR pca_9xxx_pin_get_all() {
	if (pca_9xxx) {
		return ((uint64_t)pca_9xxx->latch[4] << 32) |
			   ((uint64_t)pca_9xxx->latch[3] << 24) |
			   ((uint64_t)pca_9xxx->latch[2] << 16) |
			   ((uint64_t)pca_9xxx->latch[1] <<  8) |
			   (uint64_t)pca_9xxx->latch[0];
	} else {
		return 0;
	}
}

void pca_9xxx_isr_attach(uint8_t pin, gpio_isr_t gpio_isr, gpio_int_type_t type, void *args) {
	if (!pca_9xxx) pca9xxx_setup();

	pca_9xxx_lock();
	if (type == GPIO_INTR_DISABLE) {
		pca_9xxx->isr_func[pin] = NULL;
		pca_9xxx->isr_args[pin] = NULL;
	} else {
		pca_9xxx->isr_func[pin] = gpio_isr;
		pca_9xxx->isr_args[pin] = args;
	}

	pca_9xxx->isr_type[pin] = type;
	pca_9xxx_unlock();
}

void pca_9xxx_isr_detach(uint8_t pin) {
	if (!pca_9xxx) pca9xxx_setup();

	pca_9xxx_lock();
	pca_9xxx->isr_func[pin] = NULL;
	pca_9xxx->isr_args[pin] = NULL;
	pca_9xxx_unlock();
}

#endif

/*

 pio.pin.interrupt(40, function(value)
 	 print("value: "..value)
 end, pio.pin.IntrNegEdge)


---

pio.pin.setdir(pio.OUTPUT, 40)

while true do
  pio.pin.sethigh(40)
  tmr.delayms(200)
  pio.pin.setlow(40)
  tmr.delayms(200)
end

---

pio.pin.setdir(pio.OUTPUT, 40)

while true do
  pio.pin.inv(40)
  tmr.delayms(200)
end

---

pio.pin.setdir(pio.OUTPUT, 40)
pio.pin.setdir(pio.INPUT, 41)
pio.pin.setdir(pio.INPUT, 40)

---

pio.pin.setdir(pio.INPUT, 41)
pio.pin.setdir(pio.INPUT, 42)

pio.pin.setlow(41)

pio.pin.setlow(42)

 */

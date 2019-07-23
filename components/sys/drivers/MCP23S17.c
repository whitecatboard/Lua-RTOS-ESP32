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
 * Lua RTOS, MCP23S17 driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_GPIO_MCP23S17

/*
 * Operation functions
 */

#include "esp_attr.h"

#include <stdint.h>
#include <string.h>

#include <drivers/gpio.h>

#include <sys/status.h>
#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/spi.h>
#include <drivers/MCP23S17.h>

static MCP23S17_t *MCP23S17 = NULL;

static driver_error_t * MCP23S17_read_all_register(uint8_t reg, uint16_t *val);

/*
 * Helper functions
 */

static void MCP23S17_lock() {
    xSemaphoreTakeRecursive(MCP23S17->mtx, portMAX_DELAY);
}

static void MCP23S17_unlock() {
    xSemaphoreGiveRecursive(MCP23S17->mtx);
}

// Write 8 bits to a MCP23S17 register
static driver_error_t *MCP23S17_write8(uint8_t reg, uint8_t val) {
    uint8_t buff[3];

    buff[0] = 0x40;
    buff[1] = reg;
    buff[2] = val;

    spi_ll_select(MCP23S17->spidevice);
    spi_ll_bulk_write(MCP23S17->spidevice, sizeof(buff), buff);
    spi_ll_deselect(MCP23S17->spidevice);

    return NULL;
}

// Write 16 bits to a MCP23S17 register
static driver_error_t *MCP23S17_write16(uint8_t reg, uint16_t val) {
    uint8_t buff[4];

    buff[0] = 0x40;
    buff[1] = reg;
    buff[2] = (uint8_t)(val & 0x00ff);
    buff[3] = (uint8_t)(val >> 8);

    spi_ll_select(MCP23S17->spidevice);
    spi_ll_bulk_write(MCP23S17->spidevice, sizeof(buff), buff);
    spi_ll_deselect(MCP23S17->spidevice);

    return NULL;
}

// Read 16 bits from a MCP23S17 register
static driver_error_t *MCP23S17_read16(uint8_t reg, uint16_t *val) {
    uint8_t buff[4];

    buff[0] = 0x41;
    buff[1] = reg;
    buff[2] = 0;
    buff[3] = 0;

    spi_ll_select(MCP23S17->spidevice);
    spi_ll_bulk_rw(MCP23S17->spidevice, sizeof(buff), buff);
    spi_ll_deselect(MCP23S17->spidevice);

    *val = (buff[3] << 8) | buff[2];

    return NULL;
}

// MCP23S17 task. This task waits for a direct task notification and read all pins
// for release the MCP23S17 INT.
static void MCP23S17_task(void *arg) {
    uint8_t i, j, pin;
    uint16_t latch, current;

    for(;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        MCP23S17_lock();

        // Get current latch values
        memcpy(&latch, &MCP23S17->latch, sizeof(MCP23S17->latch));

        // Read all pins and latch
        MCP23S17_read_all_register(MCP23S17_GPIOA, &current);

        memcpy(&MCP23S17->latch, &current, sizeof(MCP23S17->latch));

        MCP23S17_unlock();

        // Process interrupts
        for(i = 0; i < MCP23S17_PORTS; i++) {
            pin = (i << 3);
            for(j = 0; j < 8;j++, pin++) {
                if (MCP23S17->isr_func[pin]) {
                    switch (MCP23S17->isr_type[pin]) {
                        case GPIO_INTR_DISABLE:
                            break;

                        case GPIO_INTR_POSEDGE:
                        case GPIO_INTR_HIGH_LEVEL:
                            if ((current & (1 << pin)) && !(latch & (1 << pin))) {
                                MCP23S17->isr_func[pin](MCP23S17->isr_args[pin]);
                            }
                            break;

                        case GPIO_INTR_NEGEDGE:
                        case GPIO_INTR_LOW_LEVEL:
                            if (!(current & (1 << pin)) && (latch & (1 << pin))) {
                                MCP23S17->isr_func[pin](MCP23S17->isr_args[pin]);
                            }
                            break;

                        case GPIO_INTR_ANYEDGE:
                            if (((current & (1 << pin))) != (latch & (1 << pin))) {
                                MCP23S17->isr_func[pin](MCP23S17->isr_args[pin]);
                            }
                            break;
                    }
                }
            }
        }
    }
}

// MCP23S17 ISR
//
// When some pin changes in MCP23S17 an interrupt is generated and it is not
// released until all pins are read.
//
// We process the interrupt as a deferred interrupt. We simply done a direct
// task notification, and the interrupt will be processed later in a task.
static void IRAM_ATTR MCP23S17_isr(void* arg) {
    portBASE_TYPE high_priority_task_awoken = 0;

    vTaskNotifyGiveFromISR(MCP23S17->task, &high_priority_task_awoken);
    if (high_priority_task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

// Read from a MCP23S17 register
static driver_error_t * MCP23S17_read_all_register(uint8_t reg, uint16_t *val) {
	MCP23S17_read16(reg, val);
    return NULL;
}

/*
 * Operation functions
 */

driver_error_t *MCP23S17_setup() {
    driver_error_t *error;
    int spidevice;

    if ((error = spi_setup(CONFIG_MCP23S17_SPI_PORT, 1, CONFIG_MCP23S17_SPI_CS, 0, 10000000, SPI_FLAG_WRITE | SPI_FLAG_READ, &spidevice))) {
        return error;
    }

    if (!MCP23S17) {
        // Create MCP23S17 data
        MCP23S17 = (MCP23S17_t *)calloc(1, sizeof(MCP23S17_t));
        if (!MCP23S17) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_NOT_ENOUGH_MEMORY, NULL);
        }

        MCP23S17->spidevice = spidevice;

        // Init mutex
        MCP23S17->mtx = xSemaphoreCreateRecursiveMutex();

        syslog(
			LOG_INFO,
			"GPIO EXTENDER %s at spi%d, cs %d",
			EXTERNAL_GPIO_NAME, CONFIG_MCP23S17_SPI_PORT, CONFIG_MCP23S17_SPI_CS
        );

        MCP23S17_lock();

        // MCP23S17 configuration as default value, and enabling hardware addressing
        MCP23S17_write8(MCP23S17_IOCON, 0b00001000);

        // Configure all pins as output / logic level 0
        MCP23S17_pin_output_mask(0, 0xff);
        MCP23S17_pin_output_mask(1, 0xff);

        MCP23S17_pin_clr_mask(0, 0xff);
        MCP23S17_pin_clr_mask(1, 0xff);

        // Configure interrupts
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
        driver_unit_lock_error_t *lock_error = NULL;

        // Lock resources
        if (CONFIG_MCP23S17_INTA > 0) {
            if ((lock_error = driver_lock(GPIO_DRIVER, 0, GPIO_DRIVER, CONFIG_MCP23S17_INTA, 0, NULL))) {
                MCP23S17_unlock();

                // Revoked lock on pin
                return driver_lock_error(GPIO_DRIVER, lock_error);
            }
        }

        if (CONFIG_MCP23S17_INTA > 0) {
            if ((lock_error = driver_lock(GPIO_DRIVER, 0, GPIO_DRIVER, CONFIG_MCP23S17_INTB, 0, NULL))) {
                MCP23S17_unlock();

                // Revoked lock on pin
                return driver_lock_error(GPIO_DRIVER, lock_error);
            }
        }
#endif

        BaseType_t xReturn = xTaskCreatePinnedToCore(MCP23S17_task, "MCP23S17", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &MCP23S17->task, xPortGetCoreID());
        if (xReturn != pdPASS) {
            MCP23S17_unlock();

            return driver_error(GPIO_DRIVER, GPIO_ERR_NOT_ENOUGH_MEMORY, NULL);
        }

        if (CONFIG_MCP23S17_INTA > 0) {
        	// Enable interrupts on all pins
        	MCP23S17_write8(MCP23S17_GPINTENA, 0xff);

        	// Pin value is compared against the previous pin value
        	MCP23S17_write8(MCP23S17_INTCONA, 0x00);

        	gpio_pin_input(CONFIG_MCP23S17_INTA);
        	gpio_pin_pullup(CONFIG_MCP23S17_INTA);
            gpio_isr_attach(CONFIG_MCP23S17_INTA, MCP23S17_isr, GPIO_INTR_NEGEDGE, NULL);
        }

        if (CONFIG_MCP23S17_INTB > 0) {
        	// Enable interrupts on all pins
        	MCP23S17_write8(MCP23S17_GPINTENB, 0xff);

        	// Pin value is compared against the previous pin value
        	MCP23S17_write8(MCP23S17_INTCONB, 0x00);

        	gpio_pin_input(CONFIG_MCP23S17_INTB);
        	gpio_pin_pullup(CONFIG_MCP23S17_INTB);
            gpio_isr_attach(CONFIG_MCP23S17_INTB, MCP23S17_isr, GPIO_INTR_NEGEDGE, NULL);
        }

        MCP23S17_unlock();

        // Read all inputs and latch it
        xTaskNotifyGive(MCP23S17->task);

        if (CONFIG_MCP23S17_INTA > 0) {
            syslog(
				LOG_INFO,
				"GPIO EXTENDER %s spi%d, INTA interrupts enabled on %s%d",
				EXTERNAL_GPIO_NAME,
				CONFIG_MCP23S17_SPI_PORT,
				gpio_portname(CONFIG_MCP23S17_INTA),
				gpio_name(CONFIG_MCP23S17_INTA)
            );
        }

        if (CONFIG_MCP23S17_INTB > 0) {
            syslog(
				LOG_INFO,
				"GPIO EXTENDER %s spi%d, INTB interrupts enabled on %s%d",
				EXTERNAL_GPIO_NAME,
				CONFIG_MCP23S17_SPI_PORT,
				gpio_portname(CONFIG_MCP23S17_INTB),
				gpio_name(CONFIG_MCP23S17_INTB)
            );
        }
    }

    return NULL;
}

driver_error_t *MCP23S17_pin_pullup(uint8_t pin) {
    uint8_t port = MCP23S17_GPIO_BANK_NUM(pin);
    uint16_t pinmask = (1 << MCP23S17_GPIO_BANK_POS(pin) << (port << 3));

    if (!MCP23S17) MCP23S17_setup();

    // Update pull. For pull-up set bit to 1.
    MCP23S17_lock();
    MCP23S17->pull |= pinmask;

    MCP23S17_write16(MCP23S17_GPPUA, MCP23S17->pull);

    // Read all pins and latch
    MCP23S17_read_all_register(MCP23S17_GPIOA, &MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t *MCP23S17_pin_nopull(uint8_t pin) {
    uint8_t port = MCP23S17_GPIO_BANK_NUM(pin);
    uint16_t pinmask = (1 << MCP23S17_GPIO_BANK_POS(pin) << (port << 3));

    if (!MCP23S17) MCP23S17_setup();

    // Update pull. For no pull-up set bit to 0.
    MCP23S17_lock();
    MCP23S17->pull &= ~pinmask;

    MCP23S17_write16(MCP23S17_GPPUA, MCP23S17->pull);

    // Read all pins and latch
    MCP23S17_read_all_register(MCP23S17_GPIOA, &MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t *MCP23S17_pin_output(uint8_t pin) {
    uint8_t port = MCP23S17_GPIO_BANK_NUM(pin);
    uint16_t pinmask = (1 << MCP23S17_GPIO_BANK_POS(pin) << (port << 3));

    if (!MCP23S17) MCP23S17_setup();

    // Update direction. For output set bit to 0.
    MCP23S17_lock();
    MCP23S17->direction &= ~pinmask;

    MCP23S17_write16(MCP23S17_IODIRA, MCP23S17->direction);

    // Read all pins and latch
    MCP23S17_read_all_register(MCP23S17_GPIOA, &MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t *MCP23S17_pin_input(uint8_t pin) {
    uint8_t port = MCP23S17_GPIO_BANK_NUM(pin);
    uint16_t pinmask = (1 << MCP23S17_GPIO_BANK_POS(pin) << (port << 3));

    if (!MCP23S17) MCP23S17_setup();

    // Update direction. For input set bit to 1.
    MCP23S17_lock();
    MCP23S17->direction |= pinmask;

    MCP23S17_write16(MCP23S17_IODIRA, MCP23S17->direction);

    // Read all pins and latch
    MCP23S17_read_all_register(MCP23S17_GPIOA, &MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t *MCP23S17_pin_set(uint8_t pin) {
    uint8_t port = MCP23S17_GPIO_BANK_NUM(pin);
    uint16_t pinmask = (1 << MCP23S17_GPIO_BANK_POS(pin) << (port << 3));

    if (!MCP23S17) MCP23S17_setup();

    // Update latch.
    MCP23S17_lock();
    MCP23S17->latch |= pinmask;

    MCP23S17_write16(MCP23S17_GPIOA, MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t *MCP23S17_pin_clr(uint8_t pin) {
    uint8_t port = MCP23S17_GPIO_BANK_NUM(pin);
    uint16_t pinmask = (1 << MCP23S17_GPIO_BANK_POS(pin) << (port << 3));

    if (!MCP23S17) MCP23S17_setup();

    // Update latch.
    MCP23S17_lock();
    MCP23S17->latch &= ~pinmask;

    MCP23S17_write16(MCP23S17_GPIOA, MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t *MCP23S17_pin_inv(uint8_t pin) {
    uint8_t port = MCP23S17_GPIO_BANK_NUM(pin);
    uint16_t pinmask = (1 << MCP23S17_GPIO_BANK_POS(pin) << (port << 3));

    if (!MCP23S17) MCP23S17_setup();

    // Update latch.
    MCP23S17_lock();
    MCP23S17->latch ^= pinmask;

    MCP23S17_write16(MCP23S17_GPIOA, MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

uint8_t MCP23S17_pin_get(uint8_t pin) {
    uint8_t port = MCP23S17_GPIO_BANK_NUM(pin);
    uint16_t pinmask = (1 << MCP23S17_GPIO_BANK_POS(pin) << (port << 3));
    uint8_t val;

    if (!MCP23S17) MCP23S17_setup();

    MCP23S17_lock();
    val = ((MCP23S17->latch & pinmask) != 0);
    MCP23S17_unlock();

    return val;
}

driver_error_t *MCP23S17_pin_pullup_mask(uint8_t port, uint8_t pinmask) {
    if (!MCP23S17) MCP23S17_setup();

    // Update pull. For pull-up set bit to 1.
    MCP23S17_lock();
    MCP23S17->pull |= (((uint16_t)pinmask) << (port << 3));

    MCP23S17_write16(MCP23S17_GPPUA, MCP23S17->pull);

    // Read all pins and latch
    MCP23S17_read_all_register(MCP23S17_GPIOA, &MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t *MCP23S17_pin_nopull_mask(uint8_t port, uint8_t pinmask) {
    if (!MCP23S17) MCP23S17_setup();

    // Update pull. For not pull-up set bit to 0.
    MCP23S17_lock();
    MCP23S17->pull &= ~(((uint16_t)pinmask) << (port << 3));

    MCP23S17_write16(MCP23S17_GPPUA, MCP23S17->pull);

    // Read all pins and latch
    MCP23S17_read_all_register(MCP23S17_GPIOA, &MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t *MCP23S17_pin_input_mask(uint8_t port, uint8_t pinmask) {
    if (!MCP23S17) MCP23S17_setup();

    // Update direction. For input set bit to 1.
    MCP23S17_lock();
    MCP23S17->direction |= (((uint16_t)pinmask) << (port << 3));

    MCP23S17_write16(MCP23S17_IODIRA, MCP23S17->direction);

    // Read all pins and latch
    MCP23S17_read_all_register(MCP23S17_GPIOA, &MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t *MCP23S17_pin_output_mask(uint8_t port, uint8_t pinmask) {
    if (!MCP23S17) MCP23S17_setup();

    // Update direction. For output set bit to 0.
    MCP23S17_lock();
    MCP23S17->direction &= ~(((uint16_t)pinmask) << (port << 3));

    MCP23S17_write16(MCP23S17_IODIRA, MCP23S17->direction);

    // Read all pins and latch
    MCP23S17_read_all_register(MCP23S17_GPIOA, &MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t * MCP23S17_pin_set_mask(uint8_t port, uint8_t pinmask) {
    if (!MCP23S17) MCP23S17_setup();

    // Update latch.
    MCP23S17_lock();
    MCP23S17->latch |= (((uint16_t)pinmask) << (port << 3));

    MCP23S17_write16(MCP23S17_GPIOA, MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t *MCP23S17_pin_clr_mask(uint8_t port, uint8_t pinmask) {
    if (!MCP23S17) MCP23S17_setup();

    // Update latch.
    MCP23S17_lock();
    MCP23S17->latch &= ~(((uint16_t)pinmask) << (port << 3));

    MCP23S17_write16(MCP23S17_GPIOA, MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

driver_error_t *MCP23S17_pin_inv_mask(uint8_t port, uint8_t pinmask) {
    if (!MCP23S17) MCP23S17_setup();

    // Update latch.
    MCP23S17_lock();
    MCP23S17->latch ^= (((uint16_t)pinmask) << (port << 3));

    MCP23S17_write16(MCP23S17_GPIOA, MCP23S17->latch);

    MCP23S17_unlock();

    return NULL;
}

void MCP23S17_pin_get_mask(uint8_t port, uint8_t pinmask, uint8_t *value) {
    if (!MCP23S17) MCP23S17_setup();

    MCP23S17_lock();
    *value = (MCP23S17->latch & (((uint16_t)pinmask) << (port << 3))) >> (port << 3);
    MCP23S17_unlock();
}

uint64_t IRAM_ATTR MCP23S17_pin_get_all() {
    if (MCP23S17) {
        return MCP23S17->latch;
    } else {
        return 0;
    }
}

void MCP23S17_isr_attach(uint8_t pin, gpio_isr_t gpio_isr, gpio_int_type_t type, void *args) {
    if (!MCP23S17) MCP23S17_setup();

    MCP23S17_lock();
    if (type == GPIO_INTR_DISABLE) {
        MCP23S17->isr_func[pin] = NULL;
        MCP23S17->isr_args[pin] = NULL;
    } else {
        MCP23S17->isr_func[pin] = gpio_isr;
        MCP23S17->isr_args[pin] = args;
    }

    MCP23S17->isr_type[pin] = type;
    MCP23S17_unlock();
}

void MCP23S17_isr_detach(uint8_t pin) {
    if (!MCP23S17) MCP23S17_setup();

    MCP23S17_lock();
    MCP23S17->isr_func[pin] = NULL;
    MCP23S17->isr_args[pin] = NULL;
    MCP23S17_unlock();
}

#endif

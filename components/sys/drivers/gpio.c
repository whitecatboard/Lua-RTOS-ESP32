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
 * Lua RTOS, gpio driver
 *
 */

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_attr.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

#include "driver/gpio.h"

#include <string.h>
#include <math.h>

#include <sys/driver.h>
#include <sys/status.h>

#include <drivers/gpio.h>
#include <drivers/pca9xxx.h>
#include <drivers/cpu.h>
#include <drivers/timer.h>

// Register drivers and errors
DRIVER_REGISTER_BEGIN(GPIO,gpio,CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS * (CPU_LAST_GPIO + 1),NULL,NULL);
    DRIVER_REGISTER_ERROR(GPIO, gpio, InvalidPinDirection, "invalid pin direction", GPIO_ERR_INVALID_PIN_DIRECTION);
    DRIVER_REGISTER_ERROR(GPIO, gpio, InvalidPin, "invalid pin", GPIO_ERR_INVALID_PIN);
    DRIVER_REGISTER_ERROR(GPIO, gpio, InvalidPort, "invalid port", GPIO_ERR_INVALID_PORT);
    DRIVER_REGISTER_ERROR(GPIO, gpio, NotEnoughtMemory, "not enough memory", GPIO_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_END(GPIO,gpio,CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS * (CPU_LAST_GPIO + 1),NULL,NULL);

/*
 * Low level gpio operations
 */

driver_error_t * IRAM_ATTR gpio_ll_pin_set(uint8_t pin) {
    if (pin < 32) {
        GPIO.out_w1ts = (1 << pin);
    } else if (pin < 40) {
        GPIO.out1_w1ts.data = (1 << (pin - 32));
    }
#if EXTERNAL_GPIO
    else {
        return pca_9xxx_pin_set(pin - 40);
    }
#endif

    return NULL;
}

driver_error_t *  IRAM_ATTR gpio_ll_pin_clr(uint8_t pin) {
    if (pin < 32) {
        GPIO.out_w1tc = (1 << pin);
    } else if (pin < 40) {
        GPIO.out1_w1tc.data = (1 << (pin - 32));
    }
#if EXTERNAL_GPIO
    else {
        return pca_9xxx_pin_clr(pin - 40);
    }
#endif

    return NULL;
}

driver_error_t *  IRAM_ATTR gpio_ll_pin_inv(int8_t pin) {
    if (pin < 32) {
        if (GPIO.out & (1 << pin)) {
            return gpio_ll_pin_clr(pin);
        } else {
            return gpio_ll_pin_set(pin);
        }
    } else if (pin < 40) {
        if (GPIO.out1.val & (1 << pin)) {
            return gpio_ll_pin_clr(pin);
        } else {
            return gpio_ll_pin_set(pin);
        }
    }
#if EXTERNAL_GPIO
    else {
        return pca_9xxx_pin_inv(pin - 40);
    }
#endif

    return NULL;
}

uint8_t IRAM_ATTR gpio_ll_pin_get(int8_t pin) {
    if (pin < 32) {
        return ((GPIO.in & (1ULL << pin)) != 0);
    } else if (pin < 40) {
        return ((GPIO.in1.data & (1ULL << (pin - 32))) != 0);
    }
#if EXTERNAL_GPIO
    else {
        return pca_9xxx_pin_get(pin - 40);
    }
#endif

    return 0;
}

/*
 * Operations over a single pin
 *
 */
driver_error_t *gpio_pin_output(uint8_t pin) {
    if (pin < 40) {
        gpio_config_t io_conf;

        // Sanity checks
        if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << pin))) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (GPIO_BIT_MASK << pin);
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;

        gpio_config(&io_conf);
    }
#if EXTERNAL_GPIO
    else {
        if (pin >= 40 + EXTERNAL_GPIO_PINS) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        driver_error_t *error;

        error = pca_9xxx_pin_output(pin - 40);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
#endif

    return NULL;
}

driver_error_t *gpio_pin_input(uint8_t pin) {
    if (pin < 40) {
        gpio_config_t io_conf;

        // Sanity checks
        if (!(GPIO_ALL_IN & (GPIO_BIT_MASK << pin))) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = (GPIO_BIT_MASK << pin);
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;

        gpio_config(&io_conf);
    }
#if EXTERNAL_GPIO
    else {
        if (pin >= 40 + EXTERNAL_GPIO_PINS) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        driver_error_t *error;

        error = pca_9xxx_pin_input(pin - 40);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
#endif

    return NULL;
}

driver_error_t *gpio_pin_set(uint8_t pin) {
    if (pin < 40) {
        // Sanity checks
        if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << pin))) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        return gpio_ll_pin_set(pin);
    }
#if EXTERNAL_GPIO
    else {
        if (pin >= 40 + EXTERNAL_GPIO_PINS) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        driver_error_t *error;

        error = pca_9xxx_pin_set(pin - 40);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
#endif

    return NULL;
}

driver_error_t *gpio_pin_clr(uint8_t pin) {
    if (pin < 40) {
        // Sanity checks
        if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << pin))) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        return gpio_ll_pin_clr(pin);
    }
#if EXTERNAL_GPIO
    else {
        if (pin >= 40 + EXTERNAL_GPIO_PINS) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        driver_error_t *error;

        error = pca_9xxx_pin_clr(pin - 40);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
#endif

    return NULL;
}

driver_error_t *gpio_pin_inv(uint8_t pin) {
    if (pin < 40) {
        // Sanity checks
        if (!(GPIO_ALL_OUT & (GPIO_BIT_MASK << pin))) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        return gpio_ll_pin_inv(pin);
    }
#if EXTERNAL_GPIO
    else {
        if (pin >= 40 + EXTERNAL_GPIO_PINS) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        driver_error_t *error;

        error = pca_9xxx_pin_inv(pin - 40);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
#endif

    return NULL;
}

driver_error_t *gpio_pin_get(uint8_t pin, uint8_t *val) {
    // Sanity checks
    if (pin < 40) {
        if (!(GPIO_ALL_IN & (GPIO_BIT_MASK << pin))) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }
    }
#if EXTERNAL_GPIO
    else {
        if (pin >= 40 + EXTERNAL_GPIO_PINS) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
#endif

    *val = gpio_ll_pin_get(pin);

    return NULL;
}

driver_error_t *gpio_pin_pullup(uint8_t pin) {
    // Sanity checks
    if (pin < 40) {
        if (!(GPIO_ALL_IN & (GPIO_BIT_MASK << pin))) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
        gpio_pullup_en(pin);
        gpio_pulldown_dis(pin);
    }
#if EXTERNAL_GPIO
    else {
        if (pin >= 40 + EXTERNAL_GPIO_PINS) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        return driver_error(GPIO_DRIVER, GPIO_ERR_PULL_UP_NOT_ALLOWED, NULL);
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
#endif

    return NULL;
}

driver_error_t *gpio_pin_pulldwn(uint8_t pin) {
    // Sanity checks
    if (pin < 40) {
        if (!(GPIO_ALL & (GPIO_BIT_MASK << pin))) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);
        gpio_pullup_dis(pin);
        gpio_pulldown_en(pin);
    }
#if EXTERNAL_GPIO
    else {
        if (pin >= 40 + EXTERNAL_GPIO_PINS) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        return driver_error(GPIO_DRIVER, GPIO_ERR_PULL_DOWN_NOT_ALLOWED, NULL);
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
#endif

    return NULL;
}

driver_error_t *gpio_pin_nopull(uint8_t pin) {
    // Sanity checks
    if (pin < 40) {
        if (!(GPIO_ALL & (GPIO_BIT_MASK << pin))) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        gpio_set_pull_mode(pin, GPIO_FLOATING);
        gpio_pullup_dis(pin);
        gpio_pulldown_dis(pin);
    }
#if EXTERNAL_GPIO
    else {
        if (pin >= 40 + EXTERNAL_GPIO_PINS) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
#endif

    return NULL;
}

/*
 * Operations over port pins
 *
 */

// Configure gpio as input using a mask
// If bit n on mask is set to 1 the gpio is configured
driver_error_t *gpio_pin_input_mask(uint8_t port, gpio_pin_mask_t pinmask) {
    if (port == 1) {
        gpio_config_t io_conf;

        // Sanity checks
        if (0xffffff0000000000 & pinmask) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        if (!(GPIO_ALL_IN & pinmask)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        if (!(GPIO_PORT_ALL & port)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
        }

        if (!(GPIO_ALL_IN & pinmask)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = pinmask;
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;

        gpio_config(&io_conf);
    }
#if EXTERNAL_GPIO
    else {
        driver_error_t *error;

        error = pca_9xxx_pin_input_mask(port - 2, (uint8_t)pinmask);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

// Configure gpio as output using a mask
// If bit n on mask is set to 1 the gpio is configured
driver_error_t *gpio_pin_output_mask(uint8_t port, gpio_pin_mask_t pinmask) {
    if (port == 1) {
        gpio_config_t io_conf;

        // Sanity checks
        if (0xffffff0000000000 & pinmask) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        if (!(GPIO_ALL_OUT & pinmask)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = pinmask;
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;

        gpio_config(&io_conf);
    }
#if EXTERNAL_GPIO
    else {
        driver_error_t *error;

        error = pca_9xxx_pin_output_mask(port - 2, (uint8_t)pinmask);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

// Set gpio pull-up using a mask
// If bit n on mask is set to 1 the gpio is set pull-up
driver_error_t *gpio_pin_pullup_mask(uint8_t port, gpio_pin_mask_t pinmask) {
    driver_error_t *error = NULL;

    if (port == 1) {
        // Sanity checks
        if (0xffffff0000000000 & pinmask) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        if (!(GPIO_ALL_IN & pinmask)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        gpio_pin_mask_t mask = GPIO_BIT_MASK;
        int i;

        for(i=0; i < GPIO_PER_PORT; i++) {
            if (pinmask & mask) {
                if ((error = gpio_pin_pullup(i))) {
                    return error;
                }
            }

            mask = (mask << 1);
        }
    }
#if EXTERNAL_GPIO
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_PULL_UP_NOT_ALLOWED, NULL);
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

// Set gpio pull-down using a mask
// If bit n on mask is set to 1 the gpio is set pull-up
driver_error_t *gpio_pin_pulldwn_mask(uint8_t port, gpio_pin_mask_t pinmask) {
    driver_error_t *error = NULL;

    if (port == 1) {
        // Sanity checks
        if (0xffffff0000000000 & pinmask) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        if (!(GPIO_ALL_IN & pinmask)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        gpio_pin_mask_t mask = GPIO_BIT_MASK;
        int i;

        for(i=0; i < GPIO_PER_PORT; i++) {
            if (pinmask & mask) {
                if ((error = gpio_pin_pulldwn(i))) {
                    return error;
                }
            }

            mask = (mask << 1);
        }
    }
#if EXTERNAL_GPIO
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_PULL_DOWN_NOT_ALLOWED, NULL);
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

// Set gpio with no pull-up and no pull-down using a mask
// If bit n on mask is set to 1 the gpio with no pull-up and no pull-down
driver_error_t *gpio_pin_nopull_mask(uint8_t port, gpio_pin_mask_t pinmask) {
    driver_error_t *error = NULL;

    if (port == 1) {
        // Sanity checks
        if (0xffffff0000000000 & pinmask) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        if (!(GPIO_ALL_IN & pinmask)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        gpio_pin_mask_t mask = GPIO_BIT_MASK;
        int i;

        for(i=0; i < GPIO_PER_PORT; i++) {
            if (pinmask & mask) {
                if ((error = gpio_pin_nopull(i))) {
                    return error;
                }
            }

            mask = (mask << 1);
        }
    }
#if EXTERNAL_GPIO
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_PULL_DOWN_NOT_ALLOWED, NULL);
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

// Put gpio on the high state using a mask
// If bit n on mask is set to 1 the gpio is put on the high state
driver_error_t *gpio_pin_set_mask(uint8_t port, gpio_pin_mask_t pinmask) {
    driver_error_t *error = NULL;

    if (port == 1) {
        // Sanity checks
        if (0xffffff0000000000 & pinmask) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        if (!(GPIO_ALL_OUT & pinmask)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        gpio_pin_mask_t mask = GPIO_BIT_MASK;
        int i;

        for(i=0; i < GPIO_PER_PORT; i++) {
            if (pinmask & mask) {
                if ((error = gpio_pin_set(i))) {
                    return error;
                }
            }

            mask = (mask << 1);
        }
    }
#if EXTERNAL_GPIO
    else {
        driver_error_t *error;

        error = pca_9xxx_pin_set_mask(port - 2, (uint8_t)pinmask);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

// Put port gpio's on the high state
// If bit n on mask is set to 1 the gpio is put on the high state
driver_error_t *gpio_port_set(uint8_t port, gpio_pin_mask_t pinmask) {
    driver_error_t *error = NULL;

    if (port == 1) {
        // Sanity checks
        if (0xffffff0000000000 & pinmask) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        if (!(GPIO_ALL_OUT & pinmask)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        gpio_pin_mask_t mask = GPIO_BIT_MASK;
        int i;

        for(i=0; i < GPIO_PER_PORT; i++) {
            if (pinmask & mask) {
                if ((error = gpio_pin_set(i))) {
                    return error;
                }
            }

            mask = (mask << 1);
        }
    }
#if EXTERNAL_GPIO
    else {
        driver_error_t *error;

        error = pca_9xxx_pin_set_mask(port - 2, (uint8_t)pinmask);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return error;
}

// Put gpio on the low state using a mask
// If bit n on mask is set to 1 the gpio is put on the low state
driver_error_t *gpio_pin_clr_mask(uint8_t port, gpio_pin_mask_t pinmask) {
    if (port == 1) {
        // Sanity checks
        if (0xffffff0000000000 & pinmask) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        if (!(GPIO_ALL_OUT & pinmask)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        driver_error_t *error = NULL;
        gpio_pin_mask_t mask = GPIO_BIT_MASK;
        int i;

        for(i=0; i < GPIO_PER_PORT; i++) {
            if (pinmask & mask) {
                if ((error = gpio_pin_clr(i))) {
                    return error;
                }
            }

            mask = (mask << 1);
        }
    }
#if EXTERNAL_GPIO
    else {
        driver_error_t *error;

        error = pca_9xxx_pin_clr_mask(port - 2, (uint8_t)pinmask);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}


// Invert gpio using a mask
// If bit n on mask is set to 1 the gpio is inverted
driver_error_t *gpio_pin_inv_mask(uint8_t port, gpio_pin_mask_t pinmask) {
    if (port == 1) {
        // Sanity checks
        if (0xffffff0000000000 & pinmask) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        if (!(GPIO_ALL_OUT & pinmask)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        driver_error_t *error = NULL;
        gpio_pin_mask_t mask = GPIO_BIT_MASK;
        int i;

        for(i=0; i < GPIO_PER_PORT; i++) {
            if (pinmask & mask) {
                if ((error = gpio_pin_inv(i))) {
                    return error;
                }
            }

            mask = (mask << 1);
        }
    }
#if EXTERNAL_GPIO
    else {
        driver_error_t *error;

        error = pca_9xxx_pin_inv_mask(port - 2, (uint8_t)pinmask);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

// Get gpio values using a mask
driver_error_t *gpio_pin_get_mask(uint8_t port, gpio_pin_mask_t pinmask, gpio_pin_mask_t *value) {
    if (port == 1) {
        // Sanity checks
        if (0xffffff0000000000 & pinmask) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        if (!(GPIO_ALL_IN & pinmask)) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN_DIRECTION, NULL);
        }

        driver_error_t *error = NULL;
        gpio_pin_mask_t mask = GPIO_BIT_MASK;
        gpio_pin_mask_t get_mask = 0;
        uint8_t val;
        int i;

        for(i=0; i < GPIO_PER_PORT; i++) {
            if (pinmask & mask) {
                if ((error = gpio_pin_get(i, &val))) {
                    return error;
                };

                get_mask |= ((gpio_pin_mask_t)val << i);
            }

            mask = (mask << 1);
        }

        *value = get_mask;
    }
#if EXTERNAL_GPIO
    else {
        pca_9xxx_pin_get_mask(port - 2, (uint8_t)pinmask, (uint8_t *)value);
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

/*
 * Operations over all port pins
 *
 */

// Configure all gpio's port as input
driver_error_t *gpio_port_input(uint8_t port) {
    if (port == 1) {
        return gpio_pin_input_mask(port, GPIO_ALL_IN);
    }
#if EXTERNAL_GPIO
    else {
        driver_error_t *error;

        error = pca_9xxx_pin_input_mask(port - 2, 0xff);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

// Configure all gpio's port as output
driver_error_t *gpio_port_output(uint8_t port) {
    if (port == 1) {
        return gpio_pin_output_mask(port, GPIO_ALL_OUT);
    }
#if EXTERNAL_GPIO
    else {
        driver_error_t *error;

        error = pca_9xxx_pin_output_mask(port - 2, 0xff);
        if (error) {
            return error;
        }
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

// Get port gpio values
driver_error_t *gpio_port_get(uint8_t port, gpio_pin_mask_t *value) {
    if (port == 1) {
        return gpio_pin_get_mask(port, GPIO_ALL_IN, value);
    }
#if EXTERNAL_GPIO
    else {
        pca_9xxx_pin_get_mask(port - 2, 0xff, (uint8_t *)value);
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

// Get all port gpio values
driver_error_t *gpio_port_get_mask(uint8_t port, gpio_pin_mask_t *value) {
    if (port == 1) {
        return gpio_pin_get_mask(port, GPIO_ALL_IN, value);
    }
#if EXTERNAL_GPIO
    else {
        pca_9xxx_pin_get_mask(port - 2, 0xff,  (uint8_t *)value);
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PORT, NULL);
    }
#endif

    return NULL;
}

driver_error_t *gpio_isr_attach(uint8_t pin, gpio_isr_t gpio_isr, gpio_int_type_t type, void *args) {
    driver_error_t *error;

    if ((error = gpio_pin_input(pin))) {
        return error;
    }

    if ((error = gpio_pin_pullup(pin))) {
        return error;
    }

    if (pin < 40) {
        // Sanity checks
        if (!(GPIO_ALL_IN & (GPIO_BIT_MASK << pin))) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INT_NOT_ALLOWED, "must be an input pin");
        }

        // Configure interrupts
        if (!status_get(STATUS_ISR_SERVICE_INSTALLED)) {
            gpio_install_isr_service(0);

            status_set(STATUS_ISR_SERVICE_INSTALLED, 0x00000000);
        }

        gpio_set_intr_type(pin, type);
        gpio_isr_handler_add(pin, gpio_isr, args);
    }
#if EXTERNAL_GPIO
    else {
        // Sanity checks
        if (pin >= 40 + EXTERNAL_GPIO_PINS) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        pca_9xxx_isr_attach(pin - 40, gpio_isr, type, args);
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
#endif

    return NULL;
}

driver_error_t *gpio_isr_detach(uint8_t pin) {
    if (pin < 40) {
        // Sanity checks
        if (!(GPIO_ALL_IN & (GPIO_BIT_MASK << pin))) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INT_NOT_ALLOWED, "must be an input pin");
        }

        gpio_isr_handler_remove(pin);
    }
#if EXTERNAL_GPIO
    else {
        // Sanity checks
        if (pin >= 40 + EXTERNAL_GPIO_PINS) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }

        pca_9xxx_isr_detach(pin - 40);
    }
#else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
#endif

    return NULL;
}

uint8_t gpio_is_input(uint8_t pin) {
    if (pin < 40) {
        return (((GPIO_BIT_MASK << pin) & GPIO_ALL_IN) != 0);
    }
#if EXTERNAL_GPIO
    else {
        return 1;
    }
#endif

    return 0;
}

uint8_t gpio_is_output(uint8_t pin) {
    if (pin < 40) {
        return (((GPIO_BIT_MASK << pin) & GPIO_ALL_OUT) != 0);
    }
#if EXTERNAL_GPIO
    else {
        return 1;
    }
#endif

    return 0;
}

int IRAM_ATTR gpio_get_pulse_time(uint8_t pin, uint8_t level, uint32_t timeout) {
    uint32_t start, end, elapsed;

    // If no timeout is provided set a 10 second timeout
    if (timeout == 0) {
        timeout = 10000000;
    }

    // Convert timeout to xthal_get_ccount units (ticks)
    timeout = timeout * (cpu_speed_hz() / 1000000.0);

    portDISABLE_INTERRUPTS();

    start = xthal_get_ccount();

    while (gpio_ll_pin_get(pin) != !level) {
        end = xthal_get_ccount();
        if ((end - start) >= timeout) {
            portENABLE_INTERRUPTS();
            return -1;
        }
    }

    while (gpio_ll_pin_get(pin) != level) {
        end = xthal_get_ccount();
        if ((end - start) >= timeout) {
            portENABLE_INTERRUPTS();
            return -1;
        }
    }

    start = xthal_get_ccount();

    while (gpio_ll_pin_get(pin) != !level) {
        end = xthal_get_ccount();
        if ((end - start) >= timeout) {
            portENABLE_INTERRUPTS();
            return -1;
        }
    }

    end = xthal_get_ccount();

    portENABLE_INTERRUPTS();

    elapsed = ((double)end - (double)start) / (double)((cpu_speed_hz() / 1000000.0));

    return elapsed;
}

/*
 * Information operations
 *
 */
const char *gpio_portname(uint8_t pin) {
    return "GPIO";
}

uint8_t gpio_name(uint8_t pin) {
    return pin;
}

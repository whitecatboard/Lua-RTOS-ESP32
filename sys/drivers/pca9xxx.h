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
 * Lua RTOS, pca9xxx driver
 *
 */

#ifndef PCA9698_H
#define	PCA9698_H

#include "luartos.h"

#if (CONFIG_GPIO_PCA9698 ||  CONFIG_GPIO_PCA9505)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_attr.h"

#include <driver/gpio.h>

#include <stdint.h>

#include <sys/driver.h>

// Number of banks
#define PCA9xxx_BANKS 5

// Number of pins
#define PCA9xxx_PINS 40

// Convert a GPIO number to it's bank number
#define PCA9xxx_GPIO_BANK_NUM(gpio) (gpio >> 3)

// Convert a GPIO number to it's position into it's bank
#define PCA9xxx_GPIO_BANK_POS(gpio) (gpio % 8)

typedef struct {
	uint8_t direction[PCA9xxx_BANKS];
	uint8_t latch[PCA9xxx_BANKS];
	gpio_isr_t isr_func[PCA9xxx_PINS];
	uint8_t isr_type[PCA9xxx_PINS];
	void *isr_args[PCA9xxx_PINS];
	TaskHandle_t task;
	SemaphoreHandle_t mtx;
	int i2cdevice;
} pca_9xxx_t;

// PCA9698 errors
#define PCA9xxx_ERR_INVALID_UNIT             (DRIVER_EXCEPTION_BASE(PCA9xxx_DRIVER_ID) |  0)

extern const int pca9xxx_erros;
extern const int pca9xxx_error_map;

driver_error_t *pca_9xxx_setup();
driver_error_t *pca_9xxx_pin_output(uint8_t pin);
driver_error_t *pca_9xxx_pin_input(uint8_t pin);
driver_error_t *pca_9xxx_pin_set(uint8_t pin);
driver_error_t *pca_9xxx_pin_clr(uint8_t pin);
driver_error_t *pca_9xxx_pin_inv(uint8_t pin);
uint8_t pca_9xxx_pin_get(uint8_t pin);
driver_error_t *pca_9xxx_pin_input_mask(uint8_t port, uint8_t pinmask);
driver_error_t *pca_9xxx_pin_output_mask(uint8_t port, uint8_t pinmask);
driver_error_t *pca_9xxx_pin_set_mask(uint8_t port, uint8_t pinmask);
driver_error_t *pca_9xxx_pin_clr_mask(uint8_t port, uint8_t pinmask);
driver_error_t *pca_9xxx_pin_inv_mask(uint8_t port, uint8_t pinmask);
void pca_9xxx_pin_get_mask(uint8_t port, uint8_t pinmask, uint8_t *value);
void pca_9xxx_isr_attach(uint8_t pin, gpio_isr_t gpio_isr, gpio_int_type_t type, void *args);
void pca_9xxx_isr_detach(uint8_t pin);
uint64_t pca_9xxx_pin_get_all();

#define gpio_ext_pin_set(a)            pca_9xxx_pin_set(a)
#define gpio_ext_pin_clr(a)            pca_9xxx_pin_clr(a)
#define gpio_ext_pin_inv(a)            pca_9xxx_pin_inv(a)
#define gpio_ext_pin_get(a)            pca_9xxx_pin_get(a)
#define gpio_ext_pin_output(a)         pca_9xxx_pin_output(a)
#define gpio_ext_pin_input(a)          pca_9xxx_pin_input(a)
#define gpio_ext_pin_set(a)            pca_9xxx_pin_set(a)
#define gpio_ext_pin_input_mask(a,b)   pca_9xxx_pin_input_mask(a,b)
#define gpio_ext_pin_output_mask(a,b)  pca_9xxx_pin_output_mask(a,b)
#define gpio_ext_pin_set_mask(a,b)     pca_9xxx_pin_set_mask(a,b)
#define gpio_ext_pin_clr_mask(a,b)     pca_9xxx_pin_clr_mask(a,b)
#define gpio_ext_pin_inv_mask(a,b)     pca_9xxx_pin_inv_mask(a,b)
#define gpio_ext_pin_get_mask(a,b,c)   pca_9xxx_pin_get_mask(a,b,c)
#define gpio_ext_pin_input_mask(a,b)   pca_9xxx_pin_input_mask(a,b)
#define gpio_ext_isr_attach(a,b,c,d)   pca_9xxx_isr_attach(a,b,c,d)
#define gpio_ext_isr_detach(a)         pca_9xxx_isr_detach(a)
#define gpio_ext_pin_get_all(a)        pca_9xxx_pin_get_all()

// PCA9XXX available GPIO pins mapped to internal pins
#define GPIO40   40
#define GPIO41   41
#define GPIO42   42
#define GPIO43   43
#define GPIO44   44
#define GPIO45   45
#define GPIO46   46
#define GPIO47   47
#define GPIO48   48
#define GPIO49   49
#define GPIO50   50
#define GPIO51   51
#define GPIO52   52
#define GPIO53   53
#define GPIO54   54
#define GPIO55   55
#define GPIO56   56
#define GPIO57   57
#define GPIO58   58
#define GPIO59   59
#define GPIO60   60
#define GPIO61   61
#define GPIO62   62
#define GPIO63   63
#define GPIO64   64
#define GPIO65   65
#define GPIO66   66
#define GPIO67   67
#define GPIO68   68
#define GPIO69   69
#define GPIO70   70
#define GPIO71   71
#define GPIO72   72
#define GPIO73   73
#define GPIO74   74
#define GPIO75   75
#define GPIO76   76
#define GPIO77   77
#define GPIO78   78
#define GPIO79   79

// PCA9XXX available pin names mapped to internal pins
#define GPIO40_NAME  "GPIO40"
#define GPIO41_NAME  "GPIO41"
#define GPIO42_NAME  "GPIO42"
#define GPIO43_NAME  "GPIO43"
#define GPIO44_NAME  "GPIO44"
#define GPIO45_NAME  "GPIO45"
#define GPIO46_NAME  "GPIO46"
#define GPIO47_NAME  "GPIO47"
#define GPIO48_NAME  "GPIO48"
#define GPIO49_NAME  "GPIO49"
#define GPIO50_NAME  "GPIO50"
#define GPIO51_NAME  "GPIO51"
#define GPIO52_NAME  "GPIO52"
#define GPIO53_NAME  "GPIO53"
#define GPIO54_NAME  "GPIO54"
#define GPIO55_NAME  "GPIO55"
#define GPIO56_NAME  "GPIO56"
#define GPIO57_NAME  "GPIO57"
#define GPIO58_NAME  "GPIO58"
#define GPIO59_NAME  "GPIO59"
#define GPIO60_NAME  "GPIO60"
#define GPIO61_NAME  "GPIO61"
#define GPIO62_NAME  "GPIO62"
#define GPIO63_NAME  "GPIO63"
#define GPIO64_NAME  "GPIO64"
#define GPIO65_NAME  "GPIO65"
#define GPIO66_NAME  "GPIO66"
#define GPIO67_NAME  "GPIO67"
#define GPIO68_NAME  "GPIO68"
#define GPIO69_NAME  "GPIO69"
#define GPIO70_NAME  "GPIO70"
#define GPIO71_NAME  "GPIO71"
#define GPIO72_NAME  "GPIO72"
#define GPIO73_NAME  "GPIO73"
#define GPIO74_NAME  "GPIO74"
#define GPIO75_NAME  "GPIO75"
#define GPIO76_NAME  "GPIO76"
#define GPIO77_NAME  "GPIO77"
#define GPIO78_NAME  "GPIO78"
#define GPIO79_NAME  "GPIO79"

// Capabilities
#define EXTERNAL_GPIO 1
#define EXTERNAL_GPIO_PINS PCA9xxx_PINS
#define EXTERNAL_GPIO_PORTS PCA9xxx_BANKS
#define EXTERNAL_GPIO_HAS_PROGRAMABLE_PULLUPS 0
#define EXTERNAL_GPIO_HAS_PROGRAMABLE_PULLDOWNS 0

#if CONFIG_GPIO_PCA9698
#define EXTERNAL_GPIO_NAME "PCA9698"
#endif

#if CONFIG_GPIO_PCA9505
#define EXTERNAL_GPIO_NAME "PCA9505"
#endif

#endif	/* PCA9698 */

#endif

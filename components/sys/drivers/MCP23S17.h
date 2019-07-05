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

#if (CONFIG_GPIO_MCP23S17)

#ifndef MCP23S17_H
#define MCP23S17_H

driver_error_t *MCP23S17_setup();
driver_error_t *MCP23S17_pin_pullup(uint8_t pin);
driver_error_t *MCP23S17_pin_output(uint8_t pin);
driver_error_t *MCP23S17_pin_input(uint8_t pin);
driver_error_t *MCP23S17_pin_set(uint8_t pin);
driver_error_t *MCP23S17_pin_clr(uint8_t pin);
driver_error_t *MCP23S17_pin_inv(uint8_t pin);
uint8_t MCP23S17_pin_get(uint8_t pin);
driver_error_t *MCP23S17_pin_pullup_mask(uint8_t port, uint8_t pinmask);
driver_error_t *MCP23S17_pin_input_mask(uint8_t port, uint8_t pinmask);
driver_error_t *MCP23S17_pin_output_mask(uint8_t port, uint8_t pinmask);
driver_error_t *MCP23S17_pin_set_mask(uint8_t port, uint8_t pinmask);
driver_error_t *MCP23S17_pin_clr_mask(uint8_t port, uint8_t pinmask);
driver_error_t *MCP23S17_pin_inv_mask(uint8_t port, uint8_t pinmask);
void MCP23S17_pin_get_mask(uint8_t port, uint8_t pinmask, uint8_t *value);
void MCP23S17_isr_attach(uint8_t pin, gpio_isr_t gpio_isr, gpio_int_type_t type, void *args);
void MCP23S17_isr_detach(uint8_t pin);
uint64_t MCP23S17_pin_get_all();

#define gpio_ext_pin_set(a)            MCP23S17_pin_set(a)
#define gpio_ext_pin_clr(a)            MCP23S17_pin_clr(a)
#define gpio_ext_pin_inv(a)            MCP23S17_pin_inv(a)
#define gpio_ext_pin_get(a)            MCP23S17_pin_get(a)
#define gpio_ext_pin_output(a)         MCP23S17_pin_output(a)
#define gpio_ext_pin_input(a)          MCP23S17_pin_input(a)
#define gpio_ext_pin_set(a)            MCP23S17_pin_set(a)
#define gpio_ext_pin_input_mask(a,b)   MCP23S17_pin_input_mask(a,b)
#define gpio_ext_pin_output_mask(a,b)  MCP23S17_pin_output_mask(a,b)
#define gpio_ext_pin_set_mask(a,b)     MCP23S17_pin_set_mask(a,b)
#define gpio_ext_pin_clr_mask(a,b)     MCP23S17_pin_clr_mask(a,b)
#define gpio_ext_pin_inv_mask(a,b)     MCP23S17_pin_inv_mask(a,b)
#define gpio_ext_pin_get_mask(a,b,c)   MCP23S17_pin_get_mask(a,b,c)
#define gpio_ext_pin_input_mask(a,b)   MCP23S17_pin_input_mask(a,b)
#define gpio_ext_isr_attach(a,b,c,d)   MCP23S17_isr_attach(a,b,c,d)
#define gpio_ext_isr_detach(a)         MCP23S17_isr_detach(a)
#define gpio_ext_pin_get_all(a)        MCP23S17_pin_get_all(a)
#define gpio_ext_pin_pullup(a)         MCP23S17_pin_pullup(a)
#define gpio_ext_pin_pullup_mask(a,b)  MCP23S17_pin_pullup_mask(a,b)

#define MCP23S17_PORTS 2
#define MCP23S17_PINS  16

// Convert a GPIO number to it's bank number
#define MCP23S17_GPIO_BANK_NUM(gpio) (gpio >> 3)

// Convert a GPIO number to it's position into it's bank
#define MCP23S17_GPIO_BANK_POS(gpio) (gpio % 8)


typedef struct {
    uint8_t direction[MCP23S17_PORTS];
    uint8_t pull[MCP23S17_PORTS];
    uint8_t latch[MCP23S17_PORTS];
    gpio_isr_t isr_func[MCP23S17_PINS];
    uint8_t isr_type[MCP23S17_PINS];
    void *isr_args[MCP23S17_PINS];
    TaskHandle_t task;
    SemaphoreHandle_t mtx;
    int spidevice;
} MCP23S17_t;

#define MCP23S17_IODIRA   0x00
#define MCP23S17_IODIRB   0x01
#define MCP23S17_GPINTENA 0x04
#define MCP23S17_GPINTENB 0x05
#define MCP23S17_INTCONA  0x08
#define MCP23S17_INTCONB  0x09
#define MCP23S17_IOCON    0x0A
#define MCP23S17_GPPUA    0x0C
#define MCP23S17_GPPUB    0x0D
#define MCP23S17_INTCAPA  0x10
#define MCP23S17_INTCAPB  0x11
#define MCP23S17_GPIOA    0x12
#define MCP23S17_GPIOB    0x13
#define MCP23S17_OLATA    0x14
#define MCP23S17_OLATB    0x15

#endif /* MCP23S17_H */

#endif

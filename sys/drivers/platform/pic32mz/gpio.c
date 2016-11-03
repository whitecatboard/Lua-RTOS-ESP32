/*
 * GPIO driver for pic32.
 *
 * Copyright (C) 2014 Serge Vakulenko, <serge@vak.ru>
 * Copyright (C) 2015 - 2016 Jaume Olivé, <jolive@iberoxarxa.com>
 * 
 * Lua RTOS, gpio driver
 *
 * Copyright (C) 2015 - 2016
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

#include "whitecat.h"

#include <sys/syslog.h>
#include <sys/drivers/cpu.h>
#include <sys/drivers/gpio.h>

static const char port_name[16] = "?ABCDEFGHJK?????";

int gpio_input_map1(int pin)
{
    switch (pin) {
        case RP('D',2):  return 0;
        case RP('G',8):  return 1;
        case RP('F',4):  return 2;
        case RP('D',10): return 3;
        case RP('F',1):  return 4;
        case RP('B',9):  return 5;
        case RP('B',10): return 6;
        case RP('C',14): return 7;
        case RP('B',5):  return 8;
        case RP('C',1):  return 10;
        case RP('D',14): return 11;
        case RP('G',1):  return 12;
        case RP('A',14): return 13;
        case RP('D',6):  return 14;
    }
    syslog(LOG_ERR, "gpio: cannot map peripheral input pin %c%d, group 1",
        port_name[pin>>4], pin & 15);
    return -1;
}

int gpio_input_map2(int pin)
{
    switch (pin) {
    case RP('D',3):  return 0;
    case RP('G',7):  return 1;
    case RP('F',5):  return 2;
    case RP('D',11): return 3;
    case RP('F',0):  return 4;
    case RP('B',1):  return 5;
    case RP('E',5):  return 6;
    case RP('C',13): return 7;
    case RP('B',3):  return 8;
    case RP('C',4):  return 10;
    case RP('D',15): return 11;
    case RP('G',0):  return 12;
    case RP('A',15): return 13;
    case RP('D',7):  return 14;
    }
    syslog(LOG_ERR, "gpio: cannot map peripheral input pin %c%d, group 2",
        port_name[pin>>4], pin & 15);
    return -1;
}

int gpio_input_map3(int pin)
{
    switch (pin) {
    case RP('D',9):  return 0;
    case RP('G',6):  return 1;
    case RP('B',8):  return 2;
    case RP('B',15): return 3;
    case RP('D',4):  return 4;
    case RP('B',0):  return 5;
    case RP('E',3):  return 6;
    case RP('B',7):  return 7;
    case RP('F',12): return 9;
    case RP('D',12): return 10;
    case RP('F',8):  return 11;
    case RP('C',3):  return 12;
    case RP('E',9):  return 13;
    }
    syslog(LOG_ERR, "gpio: cannot map peripheral input pin %c%d, group 3",
        port_name[pin>>4], pin & 15);
    return -1;
}

int gpio_input_map4(int pin)
{
    switch (pin) {
    case RP('D',1):  return 0;
    case RP('G',9):  return 1;
    case RP('B',14): return 2;
    case RP('D',0):  return 3;
    case RP('B',6):  return 5;
    case RP('D',5):  return 6;
    case RP('B',2):  return 7;
    case RP('F',3):  return 8;
    case RP('F',13): return 9;
    case RP('F',2):  return 11;
    case RP('C',2):  return 12;
    case RP('E',8):  return 13;
    }
    syslog(LOG_ERR, "gpio: cannot map peripheral input pin %c%d, group 4",
        port_name[pin>>4], pin & 15);
    return -1;
}

int gpio_is_analog_port(int pin) {
    switch (pin) {
        case RP('B',0):
        case RP('B',1):
        case RP('B',2):
        case RP('B',3):
        case RP('B',4):
        case RP('B',5):
        case RP('B',6):
        case RP('B',7):
        case RP('B',8):
        case RP('B',9):
        case RP('B',10):
        case RP('B',11):
        case RP('B',12):
        case RP('B',13):
        case RP('B',14):
        case RP('B',15):
        case RP('G',6):
        case RP('G',7):
        case RP('G',8):
        case RP('G',9):
        case RP('E',4):
        case RP('E',5):
        case RP('E',6):
        case RP('E',7):
            return 1;
    }

    return 0;
}

int gpio_port_has_analog(int port) {
    return ((port == 1) || (port == 4) || (port == 6));
}

void gpio_enable_pull_up(int pin) {
    PULLUSET(PORB(pin)) = (1 << PINB(pin));
}

void gpio_disable_pull_up(int pin) {
    PULLUCLR(PORB(pin)) = (1 << PINB(pin));    
}

void gpio_disable_analog(int pin) {
    if (gpio_is_analog_port(pin)) {
        gpio_disable_pull_up(pin);

        ANSELCLR(PORB(pin)) = (1 << PINB(pin));
    }
}

void gpio_enable_analog(int pin) {
    if (gpio_is_analog_port(pin)) {
        ANSELSET(PORB(pin)) = (1 << PINB(pin));
        gpio_pin_input(pin);
        gpio_disable_pull_up(pin);
    }
}

// Configure gpio as input using a mask
// If bit n on mask is set to 1 the gpio is configured
void gpio_pin_input_mask(unsigned int port, gpio_port_mask_t pinmask) {
	unsigned int adcmask;
	
	port--;
	
    PULLUCLR(port) = pinmask;
    PULLDPCLR(port) = pinmask;

    adcmask = cpu_port_adc_pin_mask(port + 1);
    if (adcmask) {
        ANSELCLR(port) = adcmask & pinmask;                
    }
    
    TRISSET(port) = pinmask;
    LATCLR(port) = pinmask;
}

// Configure all gpio's port as input
void gpio_port_input(unsigned int port) {
	gpio_pin_input_mask(port, cpu_port_io_pin_mask(port));
}

// Configure gpio as output using a mask
// If bit n on mask is set to 1 the gpio is configured
void gpio_pin_output_mask(unsigned int port, gpio_port_mask_t pinmask) {
	unsigned int adcmask;

	port--;
	
    PULLUCLR(port) = pinmask;
    PULLDPCLR(port) = pinmask;

    adcmask = cpu_port_adc_pin_mask(port + 1);
    if (adcmask) {
        ANSELCLR(port) = adcmask & pinmask;                
    }
                
    LATCLR(port) = pinmask;
    TRISCLR(port) = pinmask;
}

// Configure all gpio's port as output
void gpio_port_output(unsigned int port) {
	gpio_pin_output_mask(port, cpu_port_io_pin_mask(port));
}

// Set gpio pull-up using a mask
// If bit n on mask is set to 1 the gpio is set pull-up
void gpio_pin_pullup_mask(unsigned int port, gpio_port_mask_t pinmask) {
	port--;
	
    PULLUSET(port) = pinmask;
    PULLDPCLR(port) = pinmask;
}

// Set gpio pull-down using a mask
// If bit n on mask is set to 1 the gpio is set pull-up
void gpio_pin_pulldwn_mask(unsigned int port, gpio_port_mask_t pinmask) {
	port--;
	
    PULLUCLR(port) = pinmask;
    PULLDPSET(port) = pinmask;
}

// Set gpio with no pull-up and no pull-down using a mask
// If bit n on mask is set to 1 the gpio with no pull-up and no pull-down
void gpio_pin_nopull_mask(unsigned int port, gpio_port_mask_t pinmask) {
	port--;
	
    PULLUCLR(port) = pinmask;
    PULLDPCLR(port) = pinmask;
}

// Put gpio on the high state using a mask
// If bit n on mask is set to 1 the gpio is put on the high state
void gpio_pin_set_mask(unsigned int port, gpio_port_mask_t pinmask) {
	port--;
	
    LATSET(port) = pinmask;
}

// Put port gpio's on the high state
// If bit n on mask is set to 1 the gpio is put on the high state
void gpio_port_set(unsigned int port, gpio_port_mask_t pinmask) {
	port--;
	
	LAT(port) = pinmask;
}

// Put gpio on the low state using a mask
// If bit n on mask is set to 1 the gpio is put on the low state
void gpio_pin_clr_mask(unsigned int port, gpio_port_mask_t pinmask) {
	port--;
	
	LATCLR(port) = pinmask;
}

// Get gpio values using a mask
gpio_port_mask_t gpio_pin_get_mask(unsigned int port, gpio_port_mask_t pinmask) {
	port--;
	
	return (PORT(port) & pinmask);
}

// Get port gpio values
gpio_port_mask_t gpio_port_get(unsigned int port) {
	return gpio_pin_get_mask(port, cpu_port_io_pin_mask(port));
}

// Get port name
char gpio_portname(int pin) {
    return port_name[pin>>4];
}

// Get pin number
int gpio_pinno(int pin) {
    return pin & 15;
}

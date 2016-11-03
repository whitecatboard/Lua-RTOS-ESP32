/*
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

#ifndef __GPIO_H__
#define __GPIO_H__

#include <machine/pic32mz.h>

#define RP(x,n) (((x)-'A'+1) << 4 | (n))

#define ANSELCLR(port)     (*(volatile unsigned*)(0xBF860004 + (0x100 * port)))
#define ANSELSET(port)     (*(volatile unsigned*)(0xBF860008 + (0x100 * port)))
#define TRIS(port)         (*(volatile unsigned*)(0xBF860010 + (0x100 * port)))
#define TRISCLR(port)      (*(volatile unsigned*)(0xBF860014 + (0x100 * port)))
#define TRISSET(port)      (*(volatile unsigned*)(0xBF860018 + (0x100 * port)))
#define LAT(port)          (*(volatile unsigned*)(0xBF860030 + (0x100 * port)))
#define LATCLR(port)       (*(volatile unsigned*)(0xBF860034 + (0x100 * port)))
#define LATSET(port)       (*(volatile unsigned*)(0xBF860038 + (0x100 * port)))
#define LATINV(port)       (*(volatile unsigned*)(0xBF86003C + (0x100 * port)))
#define PORT(port)         (*(volatile unsigned*)(0xBF860020 + (0x100 * port)))
#define PULLU(port)        (*(volatile unsigned*)(0xBF860050 + (0x100 * port)))
#define PULLUCLR(port)     (*(volatile unsigned*)(0xBF860054 + (0x100 * port)))
#define PULLUSET(port)     (*(volatile unsigned*)(0xBF860058 + (0x100 * port)))
#define PULLUINV(port)     (*(volatile unsigned*)(0xBF86005C + (0x100 * port)))
#define PULLD(port)        (*(volatile unsigned*)(0xBF860060 + (0x100 * port)))
#define PULLDPCLR(port)    (*(volatile unsigned*)(0xBF860064 + (0x100 * port)))
#define PULLDPSET(port)    (*(volatile unsigned*)(0xBF860068 + (0x100 * port)))
#define PULLDPINV(port)    (*(volatile unsigned*)(0xBF86006C + (0x100 * port)))

#define PORB(p) ((((unsigned char)p & 0xf0) >> 4) - 1)
#define PINB(p) (((unsigned char)p & 0x0f))

#define PININPUT(port, pin)  TRISSET(port) = (1 << pin);LATCLR(port)  = (1 << pin)
#define PINOUTPUT(port, pin) LATCLR(port)  = (1 << pin);TRISCLR(port) = (1 << pin)

#define PINSET(port, pin) LATSET(port) = (1 << pin)
#define PINCLR(port, pin) LATCLR(port) = (1 << pin)
#define PININV(port, pin) LATINV(port) = (1 << pin)
#define PINGET(port, pin) ((PORT(port)) & (1 << pin)) >> pin

#define gpio_pin_input(pin)  PININPUT(PORB(pin),PINB(pin))
#define gpio_pin_output(pin) PINOUTPUT(PORB(pin),PINB(pin))

#define gpio_pin_set(pin) PINSET(PORB(pin),PINB(pin))
#define gpio_pin_clr(pin) PINCLR(PORB(pin),PINB(pin))
#define gpio_pin_inv(pin) PININV(PORB(pin),PINB(pin))
#define gpio_pin_get(pin) PINGET(PORB(pin),PINB(pin))

struct gpioreg {
    volatile unsigned ansel;            /* Analog select */
    volatile unsigned anselclr;
    volatile unsigned anselset;
    volatile unsigned anselinv;
    volatile unsigned tris;             /* Mask of inputs */
    volatile unsigned trisclr;
    volatile unsigned trisset;
    volatile unsigned trisinv;
    volatile unsigned port;             /* Read inputs, write outputs */
    volatile unsigned portclr;
    volatile unsigned portset;
    volatile unsigned portinv;
    volatile unsigned lat;              /* Read/write outputs */
    volatile unsigned latclr;
    volatile unsigned latset;
    volatile unsigned latinv;
    volatile unsigned odc;              /* Open drain configuration */
    volatile unsigned odcclr;
    volatile unsigned odcset;
    volatile unsigned odcinv;
    volatile unsigned cnpu;             /* Input pin pull-up enable */
    volatile unsigned cnpuclr;
    volatile unsigned cnpuset;
    volatile unsigned cnpuinv;
    volatile unsigned cnpd;             /* Input pin pull-down enable */
    volatile unsigned cnpdclr;
    volatile unsigned cnpdset;
    volatile unsigned cnpdinv;
    volatile unsigned cncon;            /* Interrupt-on-change control */
    volatile unsigned cnconclr;
    volatile unsigned cnconset;
    volatile unsigned cnconinv;
    volatile unsigned cnen;             /* Input change interrupt enable */
    volatile unsigned cnenclr;
    volatile unsigned cnenset;
    volatile unsigned cneninv;
    volatile unsigned cnstat;           /* Change notification status */
    volatile unsigned cnstatclr;
    volatile unsigned cnstatset;
    volatile unsigned cnstatinv;
    volatile unsigned unused[6*4];
};

int gpio_input_map1(int pin);
int gpio_input_map2(int pin);
int gpio_input_map3(int pin);
int gpio_input_map4(int pin);

void gpio_enable_analog(int pin);
void gpio_disable_analog(int pin);

char gpio_portname(int pin);
int gpio_pinno(int pin);
int gpio_is_io_port(int port);
int gpio_is_io_port_pin(int pin);
int gpio_port_has_analog(int port);

#endif

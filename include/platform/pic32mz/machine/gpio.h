/*
 * GPIO driver for pic32.
 *
 * Copyright (C) 2014 Serge Vakulenko, <serge@vak.ru>
 * Copyright (C) 2015 - 2016 Jaume Olivé, <jolive@iberoxarxa.com>
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
#ifndef _GPIO_H
#define _GPIO_H

#include <sys/ioctl.h>

/* control general-purpose i/o pins */
#define GPIO_PORT(n)	((n) & 0xff)                    /* port number */
#define GPIO_PORTA	GPIO_PORT(0)
#define GPIO_PORTB	GPIO_PORT(1)
#define GPIO_PORTC	GPIO_PORT(2)
#define GPIO_PORTD	GPIO_PORT(3)
#define GPIO_PORTE	GPIO_PORT(4)
#define GPIO_PORTF	GPIO_PORT(5)
#define GPIO_PORTG	GPIO_PORT(6)
#define GPIO_PORTH	GPIO_PORT(7)
#define GPIO_PORTJ	GPIO_PORT(8)
#define GPIO_PORTK	GPIO_PORT(9)
#define GPIO_COMMAND	0x1fff0000                      /* command mask */
#define GPIO_CONFIN	(IOC_VOID | 1 << 16 | 'g'<<8)   /* configure as input */
#define GPIO_CONFOUT    (IOC_VOID | 1 << 17 | 'g'<<8)   /* configure as output */
#define GPIO_CONFOD	(IOC_VOID | 1 << 18 | 'g'<<8)   /* configure as open drain */
#define GPIO_STORE	(IOC_VOID | 1 << 20 | 'g'<<8)   /* store all outputs */
#define GPIO_SET	(IOC_VOID | 1 << 21 | 'g'<<8)   /* set to 1 by mask */
#define GPIO_CLEAR	(IOC_VOID | 1 << 22 | 'g'<<8)   /* set to 0 by mask */
#define GPIO_INVERT	(IOC_VOID | 1 << 23 | 'g'<<8)   /* invert by mask */
#define GPIO_POLL	(IOC_VOID | 1 << 24 | 'g'<<8)   /* poll */

#endif

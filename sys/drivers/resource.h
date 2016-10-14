/*
 * Whitecat, hardware resources lock control
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

#ifndef RESOURCE_H
#define RESOURCE_H

typedef enum {RES_GPIO, RES_TIMER, RES_LORA} tresource_type;
typedef enum {RES_FREE, RES_SYSTEM, RES_STEPPER, RES_PWM, RES_UART, RES_SPI, RES_I2C} tresource_owner;

typedef struct {
    tresource_type type;
    tresource_owner owner;
    int owner_unit;
    int unit;
    int granted;
} tresource_lock;

void resource_init();
tresource_lock *resource_lock(tresource_type type, int resource_unit, tresource_owner owner, int owner_unit);
void resource_unlock(tresource_type type, int unit);

const char *resource_name(tresource_type type);
const char *resource_unit_name(tresource_type type, int unit);
const char *owner_name(tresource_owner owner);
int resource_granted(tresource_lock *lock);

#endif


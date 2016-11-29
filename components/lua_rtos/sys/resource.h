/*
 * Lua RTOS, hardware resources lock control
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

typedef enum {RES_GPIO, RES_TIMER} resource_type_t;
typedef enum {RES_FREE, RES_SYSTEM, RES_SPI, RES_STEPPER, RES_UART, RES_I2C, RES_PWM, RES_LORA} resource_owner_t;

typedef struct {
    resource_type_t type;
    resource_owner_t owner;
    int owner_unit;
    int unit;
    int granted;
} resource_lock_t;

void _resource_init();
resource_lock_t *resource_lock(resource_type_t type, int resource_unit, resource_owner_t owner, int owner_unit);
void resource_unlock(resource_type_t type, int unit);

const char *resource_name(resource_type_t type);
const char *resource_unit_name(resource_type_t type, int unit);
const char *owner_name(resource_owner_t owner);
int resource_granted(resource_lock_t *lock);

#endif


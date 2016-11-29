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

#include "lua.h"

#include <stdlib.h>
#include <sys/mutex.h>

#include <drivers/cpu.h>
#include <sys/resource.h>

#if PLATFORM_PIC32MZ

static struct mtx resource_mtx;

typedef struct {
    resource_owner_t owner;
    int unit;
} resource_lock_t_e;

static resource_lock_t_e resource_timer[9];
static resource_lock_t_e resource_pin[64];

static resource_lock_t_e *resource_lock_array(resource_type_t type) {
    switch (type) {
        case RES_GPIO: return resource_pin;
        case RES_TIMER: return resource_timer;
		default: return NULL;
    }
}

static int resource_lock_array_len(resource_type_t type) {
    switch (type) {
        case RES_GPIO: return 64;
        case RES_TIMER: return 9;
		default: return 0;
    }
}

void _resource_init() {
    int i;
    
    mtx_init(&resource_mtx, NULL, NULL, 0);

    // Pins
    for(i=0;i<resource_lock_array_len(RES_GPIO);i++) {
        resource_pin[i].owner = RES_FREE;
    }

    // Timers
    // All timers are free, except timer 1, used by system
    for(i=0;i<resource_lock_array_len(RES_TIMER);i++) {
        resource_timer[i].owner = RES_FREE;
    }
    resource_timer[0].owner = RES_SYSTEM;
    resource_timer[0].unit = 0;
}

resource_lock_t *resource_lock(resource_type_t type, int resource_unit, resource_owner_t owner, int owner_unit) {
    resource_lock_t_e *lock_array = resource_lock_array(type);
    resource_lock_t *lock;
    int granted = 0;
    
    mtx_lock(&resource_mtx);
    
    if (resource_unit == - 1) {
        // Find a free resource
        int lock_array_len = resource_lock_array_len(type);
        
        for(resource_unit = 0; resource_unit < lock_array_len; resource_unit++) {
            if (lock_array[resource_unit].owner == RES_FREE) {
                lock_array[resource_unit].owner = owner;
                lock_array[resource_unit].unit = owner_unit;
                break;
            }            
        }
        
        if (resource_unit == lock_array_len) {
            // Not a free resource
            mtx_unlock(&resource_mtx);
            return NULL;
        }

        lock = (resource_lock_t *)malloc(sizeof(resource_lock_t));

        lock->owner = lock_array[resource_unit].owner;
        
    } else {
        if (lock_array[resource_unit].owner == RES_FREE) {
            // Resource is free, granted
            lock_array[resource_unit].owner = owner;
            lock_array[resource_unit].unit = owner_unit;
            granted = 1;
        } else {
            // Resource is not free granted only if is locked by the owner
            granted = ((lock_array[resource_unit].owner == owner) &&
                       lock_array[resource_unit].unit == owner_unit);
        }

        lock = (resource_lock_t *)malloc(sizeof(resource_lock_t));
    
        lock->owner = lock_array[resource_unit].owner;
        lock->owner_unit = lock_array[resource_unit].unit;
        lock->granted = granted;
    }
    
    lock->type = type;
    lock->unit = resource_unit;
    mtx_unlock(&resource_mtx);
    
    return lock;
}

void resource_unlock(resource_type_t type, int unit) {
    resource_lock_t_e *lock_array = resource_lock_array(type);
    
    mtx_lock(&resource_mtx);
    
    lock_array[unit].owner = RES_FREE;
    lock_array[unit].unit = 0;
    
    mtx_unlock(&resource_mtx);    
}
#else
void _resource_init() {

}
#endif

const char *resource_name(resource_type_t type) {
    switch (type) {
        case RES_GPIO:  return "pin";
        case RES_TIMER: return "timer";
		default:        return "";
    }
}

const char *resource_unit_name(resource_type_t type, int unit) {
    switch (type) {
        case RES_GPIO:  return cpu_pin_name(unit);
        case RES_TIMER: return "timer";
		default:        return "";
    }
}

const char *owner_name(resource_owner_t owner) {
    switch (owner) {
        case RES_SYSTEM:  return "LuaOS";
        case RES_STEPPER: return "stepper";
        case RES_PWM:     return "pwm";
        case RES_UART:    return "uart";
        case RES_SPI:     return "spi";
        case RES_I2C:     return "i2c";
        case RES_LORA:    return "lora";
		default:          return "";
    }
}

int resource_granted(resource_lock_t *lock) {
    return (lock && lock->granted);
}

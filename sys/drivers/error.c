/*
 * Whitecat, hardware error functions
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

#include <stdlib.h>
#include <sys/drivers/error.h>

tdriver_error *lock_error(tresource_lock *lock) {
    tdriver_error *error;

    error = (tdriver_error *)malloc(sizeof(tdriver_error));
    if (error) {
        error->type = LOCK;
        error->resource = lock->type;
        error->resource_unit = lock->unit;          
        error->owner = lock->owner;
        error->owner_unit = lock->owner_unit;
    }

    free(lock);

    return error;
}

tdriver_error *setup_error(tresource_type resource, const char *msg) {
    tdriver_error *error;

    error = (tdriver_error *)malloc(sizeof(tdriver_error));
    if (error) {
        error->type = SETUP;
        error->resource = resource;
        error->msg = msg;
    }

    return error;
}
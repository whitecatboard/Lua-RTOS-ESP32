/*
 * Lua RTOS, Lua helper functions for throw an error from a driver error
 *
 * Copyright (C) 2015 - 2017
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

#include "error.h"
#include "lauxlib.h"

#include <string.h>
#include <stdlib.h>

#include <sys/driver.h>

int luaL_driver_error(lua_State* L, driver_error_t *error) {
	driver_error_t err;
    int ret_val;
    
    bcopy(error, &err, sizeof(driver_error_t));
    free(error);
    
    if (err.type == LOCK) {
        ret_val = luaL_error(L,
            "%s%d, %s%d is used by %s%d",
			err.lock_error->owner_driver->name,
			err.lock_error->owner_unit,
			err.lock_error->target_driver->name,
			err.lock_error->target_unit,
			err.lock_error->lock->owner->name,
			err.lock_error->lock->unit
		);
        
        free(err.lock_error);

        return ret_val;
    } else if (err.type == SETUP) {
    	if (err.msg) {
            ret_val = luaL_error(L,
                "%d:%s (%s)",
    			err.exception,
    			driver_get_err_msg(&err),
                err.msg
            );
    	} else {
            ret_val = luaL_error(L,
                "%d:%s",
    			err.exception,
    			driver_get_err_msg(&err)
            );
    	}
    } else if (err.type == OPERATION) {
    	if (err.msg) {
            ret_val = luaL_error(L,
                "%d:%s (%s)",
    			err.exception,
    			driver_get_err_msg(&err),
                err.msg
            );
    	} else {
            ret_val = luaL_error(L,
                "%d:%s",
    			err.exception,
    			driver_get_err_msg(&err)
            );
    	}
    }
    
    return luaL_error(L, driver_get_err_msg(error));
}

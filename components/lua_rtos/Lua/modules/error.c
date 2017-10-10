/*
 * Lua RTOS, Lua helper functions for throw an error from a driver error
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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
    int ret_val;
    
    // Copy relevant information about the error in the stack space.
    //
    // This is needed because *error is created in the heap and must be
    // destroy, but relevant information is used when calling to the
    // luaL_error interrupts the program flow and instructions after this
    // call are not executed, so any free call after this call is not
    // executed.
    const char *msg = NULL;
    const char *ext_msg = NULL;
    const char *target_name;
    const char *owner_name;
    int target_unit;
    int owner_unit;
    int exception;

    int error_type = error->type;

    if (error_type == LOCK) {
    	target_name = error->lock_error->target_driver->name;
		owner_name = error->lock_error->lock->owner->name;
		target_unit = error->lock_error->target_unit;
		owner_unit = error->lock_error->lock->unit;

		free(error->lock_error);
    } else if (error_type == OPERATION) {
    	msg = driver_get_err_msg(error);

    	if (error->msg) {
    		ext_msg = error ->msg;
    	}

    	exception = error->exception;
    }
    
    free(error);

    if (error_type == LOCK) {
        ret_val = luaL_error(L,
            "%s%d is used by %s%d",
			target_name,
			target_unit,
			owner_name,
			owner_unit
		);
        
        return ret_val;
    } else if (error_type == OPERATION) {
    	if (ext_msg) {
            ret_val = luaL_error(L,
                "%d:%s (%s)",
    			exception,
				msg,
				ext_msg
            );
    	} else {
            ret_val = luaL_error(L,
                "%d:%s",
    			exception,
				msg
            );
    	}
    }
    
    return luaL_error(L, driver_get_err_msg(error));
}

int luaL_deprecated(lua_State* L, const char *old, const char *new) {
	// Build the deprecated message
	luaL_where(L, 1);
	lua_pushfstring(L, "WARNING %s is deprecated, please use %s instead\r\n", old, new);
	lua_concat(L, 2);

	printf(luaL_checkstring (L, -1));

	// Remove string from the stack
	lua_pop(L,1);

	return 0;
}

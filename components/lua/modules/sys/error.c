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
 * Lua RTOS, Lua CPU helper functions for throw an error from a driver error
 *
 */

#include "sdkconfig.h"
#include "error.h"
#include "lauxlib.h"

#include <string.h>
#include <stdlib.h>

#include <sys/driver.h>

int luaL_driver_error(lua_State* L, driver_error_t *error) {
    int ret_val;
    
    // We must copy relevant information about the error in the stack space.
    //
    // This is needed because *error is created in the heap and must be
    // destroy, but relevant information is used when calling to the
    // luaL_error that interrupts the program flow and instructions after this
    // call are not executed, so any free call after this call is not
    // executed.
    const char *msg = NULL;
    const char *ext_msg = NULL;
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    const char *target_name;
    const char *owner_name;
    int target_unit;
    int owner_unit;
#endif
    int exception;

    int error_type = error->type;

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    if (error_type == LOCK) {
    	target_name = error->lock_error->target_driver->name;
		owner_name = error->lock_error->lock->owner->name;
		target_unit = error->lock_error->target_unit;
		owner_unit = error->lock_error->lock->unit;

		if (strcmp(error->lock_error->lock->owner->name, "spi") == 0) {
			owner_unit = (owner_unit & 0xff00) >> 8;
		} else if (strcmp(error->lock_error->lock->owner->name, "i2c") == 0) {
			owner_unit = (owner_unit & 0xff00) >> 8;
		}

		free(error->lock_error);
	    free(error);

        ret_val = luaL_error(L,
            "%s%d is used by %s%d",
			target_name,
			target_unit,
			owner_name,
			owner_unit
		);
    } else
#endif
    if (error_type == OPERATION) {
    	msg = driver_get_err_msg(error);

    	if (error->msg) {
    		ext_msg = error ->msg;
    	}

    	exception = error->exception;

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
    } else {
    	msg = driver_get_err_msg(error);

    	ret_val = luaL_error(L, msg);
    }
    
    return ret_val;
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

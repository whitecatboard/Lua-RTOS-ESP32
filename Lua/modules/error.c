#include "error.h"

#include "lauxlib.h"

#include <string.h>
#include <stdlib.h>

int luaL_driver_error(lua_State* L, const char *msg, tdriver_error *error) {
    tdriver_error err;
    int ret_val;
    
    bcopy(error, &err, sizeof(tdriver_error));
    free(error);
    
    if (err.type == LOCK) {
        if (err.resource_unit == -1) {
            ret_val = luaL_error(L,
                "%s, no %s available", 
                msg,
                resource_name(err.resource)
            );                        
        } else {
            ret_val = luaL_error(L,
                "%s, %s is used by %s%d", 
                msg,
                resource_unit_name(err.resource, err.resource_unit),
                owner_name(err.owner),
                err.owner_unit + 1
            );            
        }
        
        return ret_val;
    } else if (err.type == SETUP) {
        ret_val = luaL_error(L,
            "%s, %s (%s)", 
            msg,
            resource_name(err.resource),
            err.msg
        );                                
    }
    
    return luaL_error(L, msg);
}
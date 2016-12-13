#include "lua.h"
#include "lauxlib.h"

int stackDump(lua_State *L) {
    int i;
    int top = lua_gettop(L);

    printf("top %d\n", top);
    printf("------------------------\n");

    for (i = 1; i <= top; i++) {  /* repeat for each level */
      int t = lua_type(L, i);
      switch (t) {

        case LUA_TSTRING:  /* strings */
          printf("%d: ", i);
          printf("`%s'\n", lua_tostring(L, i));
          break;

        case LUA_TBOOLEAN:  /* booleans */
          printf("%d: ", i);
          printf(lua_toboolean(L, i) ? "true" : "false");
          printf("\n");
          break;

        case LUA_TNUMBER:  /* numbers */
          printf("%d: ", i);
          printf("%g\n", lua_tonumber(L, i));
          break;

        default:  /* other values */
          printf("%d: ", i);
          printf("%s\n", lua_typename(L, t));
          break;

      }
    }
    printf("\n\n");  /* end the listing */

    return 0;
}

static int luaB_try (lua_State *L) {
    int total = lua_gettop(L);
    
    int tryFunc = LUA_NOREF, catchFunc = LUA_NOREF, finallyFunc = LUA_NOREF;

    if (total == 3) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        finallyFunc = luaL_ref(L, LUA_REGISTRYINDEX);
        total--;
    }
    
    if (total == 2) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        catchFunc = luaL_ref(L, LUA_REGISTRYINDEX);
        total--;        
    }

    luaL_checktype(L, 1, LUA_TFUNCTION);
    tryFunc = luaL_ref(L, LUA_REGISTRYINDEX);

    // Call try function
    lua_rawgeti(L, LUA_REGISTRYINDEX, tryFunc);
    
    int status = lua_pcall(L, 0, 0, 0); 
    if (status != LUA_OK) {
        if (catchFunc != LUA_NOREF) {
            const char *msg = lua_tostring(L, -1);
            
            // Parse error message, with format
            // where:line-number error-code:error-message
            //
            // where and line-number are mandatory
            // error-code is optional
            const char *whereEnd = strchr(msg,':');
            const char *lineEnd = strchr(whereEnd + 1,':');
            const char *messageStart;
            
            int error_code;
            
            if (!sscanf(lineEnd + 2,"%d:", &error_code)) {
                error_code = 0;
                messageStart = lineEnd + 2;
            } else {
                messageStart = strchr(lineEnd + 2,':');
                messageStart++;
            }
            
            lua_remove(L, -2);
                        
            // Call fails, call to catch function
            lua_rawgeti(L, LUA_REGISTRYINDEX, catchFunc); 
            
            // Push where (where error occurs)
            lua_pushlstring(L, msg, whereEnd - msg);
            
            // Push line (line number where error occurs)
            lua_pushlstring(L, whereEnd + 1, lineEnd -whereEnd - 1);
            
            // Push error code
            if (!error_code) {
                // No error code, push nil value
                lua_pushnil(L);
            } else {
                lua_pushinteger(L, error_code);
            }
            
            // Push error message
            lua_pushstring(L, messageStart);
            
            status = lua_pcall(L, 4, 0, 0);    
			if (status != LUA_OK) {
				const char *msg = lua_tostring(L, -1);

			    if ( tryFunc!= LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, tryFunc);
			    if ( catchFunc!= LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, catchFunc);
			    if ( finallyFunc!= LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, finallyFunc);
				
				luaL_error(L, msg);
			}			
        }
    }
    
    if (finallyFunc != LUA_NOREF) {
       // Call finally
        lua_rawgeti(L, LUA_REGISTRYINDEX, finallyFunc);
        status = lua_pcall(L, 0, 0, 0); 
		if (status != LUA_OK) {
			const char *msg = lua_tostring(L, -1);

		    if ( tryFunc!= LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, tryFunc);
		    if ( catchFunc!= LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, catchFunc);
		    if ( finallyFunc!= LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, finallyFunc);
			
			luaL_error(L, msg);
		}			
    }
    
    if ( tryFunc!= LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, tryFunc);
    if ( catchFunc!= LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, catchFunc);
    if ( finallyFunc!= LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, finallyFunc);
    
    return 0;
}

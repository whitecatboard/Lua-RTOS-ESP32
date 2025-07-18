if(CONFIG_LUA_RTOS_USE_HTTP_SERVER)
	set(srcs
	    "captivedns.c"
	)
	
	set(include_dirs ".")
else()
	set(srcs "")
	set(include_dirs "")
endif()

idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES sys
)

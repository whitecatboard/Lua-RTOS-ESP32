if(CONFIG_LUA_RTOS_USE_LFS)
	set(srcs
	    "lfs_util.c"
	    "lfs.c"
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

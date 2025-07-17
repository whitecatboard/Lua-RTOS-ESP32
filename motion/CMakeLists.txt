if(CONFIG_LUA_RTOS_LUA_USE_STEPPER)
	set(srcs
	    "motion_math.c"
	    "motion.c"
	    "s_curve_motion.c"
	)
	
	set(include_dirs ".")
else()
	set(srcs "")
	set(include_dirs "")
endif()

idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES spi_flash sys
)

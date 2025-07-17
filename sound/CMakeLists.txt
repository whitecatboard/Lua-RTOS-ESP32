if(CONFIG_LUA_RTOS_LUA_USE_SOUND)
	set(srcs
	    "sound.c"
	    "tone_dac.c"
	    "tone_pwm.c"
	    "tone.c"
	)
	
	set(include_dirs ".")
else()
	set(srcs "")
	set(include_dirs "")
endif()

idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES sys driver
)

if(CONFIG_LUA_RTOS_LUA_USE_RCSWITCH)
	set(srcs
	    "arduino.c"
	    "RCSwitch.c"
	    "rc-switch.c"
	)
	
	set(include_dirs ".")
else()
	set(srcs "")
	set(include_dirs "")
endif()

set(fragments
    "iram.lf"
)	   

idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
    LDFRAGMENTS "${fragments}"
    REQUIRES sys esp_timer
)

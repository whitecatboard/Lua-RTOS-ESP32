if(CONFIG_LUA_RTOS_USE_HTTP_SERVER)
    set(COMPONENT_ADD_FS
		"fs"
	)
else()
	set(COMPONENT_ADD_FS "")
endif()
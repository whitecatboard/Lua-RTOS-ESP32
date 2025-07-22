#if(CONFIG_LUA_RTOS_LUA_USE_CJSON)
    set(COMPONENT_ADD_FS
		"fs"
	)
#else()
#	set(COMPONENT_ADD_FS "")
#endif()
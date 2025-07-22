if(CONFIG_LUA_RTOS_USE_SPIFFS)
	set(srcs
	    "esp_spiffs.c"
	    "spiffs_cache.c"
	    "spiffs_check.c"
	    "spiffs_gc.c"
	    "spiffs_hydrogen.c"
	    "spiffs_nucleus.c"
	)
	
	set(include_dirs ".")
else()
	set(srcs "")
	set(include_dirs "")
endif()

idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES spi_flash sys esp_partition
)
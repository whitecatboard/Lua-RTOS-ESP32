set(srcs
    "nmea0183.c"
)
	
set(include_dirs ".")

idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
)

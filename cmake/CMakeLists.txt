set(srcs "")
set(include_dirs "")
 
idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES partition_table
)

include(parts.cmake)
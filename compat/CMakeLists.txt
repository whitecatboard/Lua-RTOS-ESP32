set(srcs 
    "getnameinfo.c"
    "gai_strerror.c"
)

set(include_dirs 
    "include"
)

idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
)

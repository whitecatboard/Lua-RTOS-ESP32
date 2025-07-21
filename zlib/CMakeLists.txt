set(srcs
    "adler32.c"
    "compress.c"
    "crc32.c"
    "deflate.c"
    "gzclose.c"
    "gzlib.c"
    "gzread.c"
    "gzwrite.c"
    "infback.c"
    "inffast.c"
    "inflate.c"
    "inftrees.c"
    "trees.c"
    "uncompr.c"
    "zutil.c"
)

set(include_dirs ".")

idf_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${include_dirs}"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE PPM_SUPPORTED)
target_compile_definitions(${COMPONENT_LIB} PRIVATE HAVE_MATH_H)
target_compile_definitions(${COMPONENT_LIB} PRIVATE HAVE_CTYPE_H)
target_compile_definitions(${COMPONENT_LIB} PRIVATE HAVE_UNISTD_H)
target_compile_definitions(${COMPONENT_LIB} PRIVATE HAVE_ERRNO_H)
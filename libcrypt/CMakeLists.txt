set(srcs
	"crypt-md5.c"
	"crypt.c"
	"misc.c"
)

set(include_dirs ".")

idf_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${include_dirs}"
	PRIV_REQUIRES mbedtls
)
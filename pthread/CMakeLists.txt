set(srcs
	"_pthread.c"
	"attr.c"
	"cond.c"
	"create.c"
	"join.c"
	"key.c"
	"kill.c"
	"mutex.c"
	"once.c"
	"self.c"
	"port/esp_pthread.c"
)

set(include_dirs "${COMPONENT_DIR}" "port")

set(fragments
    "iram.lf"
)	   

idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
	LDFRAGMENTS "${fragments}"
    REQUIRES "sys"
)

set(extra_link_flags "-u pthread_create")
list(APPEND extra_link_flags "-u pthread_join")
list(APPEND extra_link_flags "-u pthread_attr_init")
list(APPEND extra_link_flags "-u pthread_cond_init")
list(APPEND extra_link_flags "-u pthread_key_create")
list(APPEND extra_link_flags "-u pthread_kill")
list(APPEND extra_link_flags "-u pthread_mutex_init")
list(APPEND extra_link_flags "-u pthread_once")
list(APPEND extra_link_flags "-u pthread_self")
list(APPEND extra_link_flags "-u _pthread_init")

target_link_libraries(${COMPONENT_LIB} INTERFACE "${extra_link_flags}")
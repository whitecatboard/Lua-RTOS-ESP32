#
# Component Makefile
#
COMPONENT_ADD_LDFLAGS = -Wl,--whole-archive -l$(COMPONENT_NAME) -Wl,--no-whole-archive -L$(COMPONENT_PATH)/ld -T lua_rtos.ld
CFLAGS += -D_POSIX_THREADS=1 -D_UNIX98_THREAD_MUTEX_ATTRIBUTES=1


COMPONENT_SRCDIRS := . luartos_build.h freertos vfs editor sys unix syscalls math drivers lmic sensors \
					   sys/machine pthread Lua/common Lua/modules Lua/platform Lua/src lwip/netif \
					   lwip

COMPONENT_ADD_INCLUDEDIRS := . .. Lua/adds Lua/common Lua/modules Lua/platform Lua/src ./lwip/include
							   ./../ssd1306 
							   
COMPONENT_PRIV_INCLUDEDIRS :=

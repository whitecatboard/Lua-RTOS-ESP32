#
# Component Makefile
#
COMPONENT_ADD_LDFLAGS = -Wl,--whole-archive -l$(COMPONENT_NAME) -Wl,--no-whole-archive -L$(COMPONENT_PATH)/ld -T lua_rtos.ld

COMPONENT_SRCDIRS := . luartos_build.h freertos vfs editor sys unix syscalls math drivers lmic sensors \
					   sys/machine pthread Lua/common Lua/modules Lua/platform Lua/src lwip/netif \
					   lwip shell

COMPONENT_ADD_INCLUDEDIRS := . .. Lua/adds Lua/common Lua/modules Lua/platform Lua/src ./lwip/include
							   
COMPONENT_PRIV_INCLUDEDIRS :=

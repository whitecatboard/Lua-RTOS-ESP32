#
# Component Makefile
#

COMPONENT_ADD_LDFLAGS = -Wl,--whole-archive -l$(COMPONENT_NAME) -Wl,--no-whole-archive -L$(COMPONENT_PATH)/ld -T lua_rtos.ld -llua_rtos

COMPONENT_SRCDIRS := . luartos_build.h freertos vfs editor sys unix syscalls math drivers lmic sensors \
					   sys/machine pthread Lua/common Lua/modules Lua/platform Lua/src lwip

COMPONENT_ADD_INCLUDEDIRS := . ./../spiffs include/freertos Lua/adds Lua/common Lua/modules Lua/platform Lua/src \
							   lmic ./../ vfs
							   
COMPONENT_PRIV_INCLUDEDIRS := 
#
# Component Makefile
#

COMPONENT_SRCDIRS := . luartos_build.h freertos vfs editor sys unix syscalls math drivers lmic \
					   sys/machine pthread Lua/common Lua/modules Lua/platform Lua/src lwip

COMPONENT_ADD_INCLUDEDIRS := . ./../spiffs include/freertos Lua/adds Lua/common Lua/modules Lua/platform Lua/src \
							   lmic ./../
							   
COMPONENT_PRIV_INCLUDEDIRS := 
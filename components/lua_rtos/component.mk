#
# Component Makefile
#

COMPONENT_SRCDIRS := . freertos vfs editor sys unix syscalls math drivers sys/machine pthread Lua/common Lua/modules Lua/platform Lua/src

COMPONENT_ADD_INCLUDEDIRS := . ./../spiffs include/freertos Lua/adds Lua/common Lua/modules Lua/platform Lua/src
COMPONENT_PRIV_INCLUDEDIRS := 

include $(IDF_PATH)/make/component_common.mk

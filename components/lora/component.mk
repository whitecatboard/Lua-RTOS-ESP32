#
# Component Makefile
#

ifdef CONFIG_LUA_RTOS_LUA_USE_LORA

COMPONENT_SRCDIRS := ./gateway/src ./node/lmic ./common
COMPONENT_ADD_INCLUDEDIRS := ./gateway/inc ./node/lmic ./common

else

# disable LORA support
COMPONENT_SRCDIRS :=
COMPONENT_ADD_INCLUDEDIRS :=

endif

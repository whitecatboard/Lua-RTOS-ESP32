#
# Component Makefile
#

ifdef CONFIG_LUA_RTOS_LUA_USE_LORA

COMPONENT_SRCDIRS := ./gateway/multi_channel/src ./gateway/single_channel ./node/lmic ./common
COMPONENT_ADD_INCLUDEDIRS := ./gateway/multi_channel/inc ./gateway/single_channel ./node/lmic ./common

else

# disable LORA support
COMPONENT_SRCDIRS :=
COMPONENT_ADD_INCLUDEDIRS :=

endif

#
# Component Makefile
#

ifdef CONFIG_LUA_RTOS_LUA_USE_SCP_NET

COMPONENT_SRCDIRS := ./src
COMPONENT_ADD_INCLUDEDIRS := ./src  ./include

else

# disable SCP support
COMPONENT_SRCDIRS :=
COMPONENT_ADD_INCLUDEDIRS :=

endif 

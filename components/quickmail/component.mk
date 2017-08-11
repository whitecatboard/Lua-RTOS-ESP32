#
# Component Makefile
#

ifdef CONFIG_LUA_RTOS_LUA_USE_CURL_NET

COMPONENT_SRCDIRS := . 
COMPONENT_ADD_INCLUDEDIRS := . 

else

# disable quickmail support
COMPONENT_SRCDIRS :=
COMPONENT_ADD_INCLUDEDIRS :=

endif

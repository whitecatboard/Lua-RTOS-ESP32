#
# Component Makefile
#

ifdef CONFIG_LUA_RTOS_LUA_USE_THREAD

COMPONENT_SRCDIRS := . test

else

# disable THREAD support
COMPONENT_SRCDIRS :=

endif

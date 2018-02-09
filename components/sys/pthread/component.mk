#
# Component Makefile
#

ifdef CONFIG_LUA_RTOS_LUA_USE_THREAD

CFLAGS += -D_POSIX_THREADS=1

COMPONENT_SRCDIRS := . test

else

# disable THREAD support
COMPONENT_SRCDIRS :=

endif

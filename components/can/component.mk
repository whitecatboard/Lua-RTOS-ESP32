# currently the only SoC supported; to be moved into Kconfig

ifdef CONFIG_LUA_RTOS_LUA_USE_CAN

COMPONENT_SRCDIRS := . cfg
COMPONENT_ADD_INCLUDEDIRS := include

else

# disable CAN support
COMPONENT_SRCDIRS :=
COMPONENT_ADD_INCLUDEDIRS :=

endif

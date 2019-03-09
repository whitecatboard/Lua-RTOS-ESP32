#
# Component Makefile
#

ifdef CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

COMPONENT_SRCDIRS := . fonts primitives image color qrcodegen
COMPONENT_ADD_INCLUDEDIRS := . .. fonts primitives image color qrcodegen
COMPONENT_PRIV_INCLUDEDIRS := 

else

# disable GDISPLAY support
COMPONENT_SRCDIRS :=
COMPONENT_ADD_INCLUDEDIRS :=

endif

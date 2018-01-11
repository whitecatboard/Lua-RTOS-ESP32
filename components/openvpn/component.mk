ifdef CONFIG_LUA_RTOS_USE_OPENVPN
COMPONENT_SRCDIRS := . src/compat src/openvpn src/plugins
COMPONENT_ADD_INCLUDEDIRS := . src/compat src/openvpn src/plugins include ../
else
COMPONENT_SRCDIRS :=
COMPONENT_ADD_INCLUDEDIRS :=
endif
INC_DIRS += $(Lua_ROOT)adds $(Lua_ROOT)common $(Lua_ROOT)modules $(Lua_ROOT)src
CFLAGS += -DKERNEL -DLUA_USE_CTYPE -DLUA_32BITS -DLUA_USE_ROTABLE=1

Lua_SRC_DIR = $(Lua_ROOT)src $(Lua_ROOT)modules $(Lua_ROOT)common 

$(eval $(call component_compile_rules,Lua))
